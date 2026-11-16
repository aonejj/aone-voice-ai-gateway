#include <iostream>
#include <grpcpp/grpcpp.h>

#include "../RTCAgentBridge/AgentEgressAudioSinkInterface.h"
#include "GRPCChannelInterface.h"
#include "GRPCAgentStream.h"

GRPCAgentStream::GRPCAgentStream(const std::string& id, GRPCChannelInterface* channel) 
 : _id(id),
   _channel_ptr(channel)  {

 }

GRPCAgentStream::~GRPCAgentStream() {
    std::cout<<"~GRPCAgentStream" << std::endl;
}

int32_t GRPCAgentStream::CreateStream() {
    auto* stub = static_cast<voice::agent::v1::VoiceAgentService::Stub*>(_channel_ptr->GetStubPtr());
    auto* cq = static_cast<grpc::CompletionQueue*>(_channel_ptr->GetCQPtr());
    std::lock_guard<std::mutex> lock(_mutex);
    if (_stream || _closing) {
        return -1;
    }

    _stream = stub->PrepareAsyncAudioStream(&_context, cq);
    _start_pending = true;
    _stream->StartCall(new OpTag(shared_from_this(), OpType::START));

    return 0;    
}

void GRPCAgentStream::Release() {
    std::cout<<"GRPCAgentStream::Release+++"<< std::endl;
  std::lock_guard<std::mutex> lock(_mutex);
  if (_closing) {
      return;
  }
  _closing = true;

  if (!_stream) {
      return;
  }
  _context.TryCancel();
  _maybe_finish_l();  
  std::cout<<"GRPCAgentStream::Release---" << std::endl;
}

void GRPCAgentStream::SendAudioFrame(const uint8_t *pcm, size_t size) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_closing) {
        return;
    }

    if (_write_queue.size() >= kMaxWriteQueue) {
        _write_queue.pop_front();
    }

    std::vector<int16_t> frame(size / sizeof(int16_t));
    std::memcpy(frame.data(), pcm, frame.size() * sizeof(int16_t));
    _write_queue.push_back(std::move(frame));
    _kick_write_l();
}

void GRPCAgentStream::SetSink(AgentEgressAudioSinkInterface* sink) {
    std::lock_guard<std::mutex> lock(_sink_mutex);
    _sink = sink;
}

void GRPCAgentStream::OnComplete(OpType op, bool ok) {
    switch (op) {
    case OpType::START:  _on_start(ok);  break;
    case OpType::READ:   _on_read_frame(ok);   break;
    case OpType::WRITE:  _on_write_frame(ok);  break;
    case OpType::FINISH: _on_finish(ok); break;
    }
}   

void GRPCAgentStream::_on_start(bool ok) {
  std::lock_guard<std::mutex> lock(_mutex);
  _start_pending = false;

  if (!ok) {
      _closing = true; 
  } else {
      _started = true;
      if (!_closing) {
          _issue_read_l();
          _kick_write_l(); 
      }
  }
  _maybe_finish_l();
}

void GRPCAgentStream::_on_read_frame(bool ok) {
//    std::cout << "GRPCAgentStream::_on_read_frame ok "<< ok << std::endl;
  bool deliver = false;
  {
      std::lock_guard<std::mutex> lock(_mutex);
      deliver = ok && !_closing;
  }

  if (deliver) {
      std::lock_guard<std::mutex> lock(_sink_mutex);
      if (_sink) {
          const auto& pcm = _read_frame.pcm_s16le();
          _sink->PushPcm(
              reinterpret_cast<const int16_t*>(pcm.data()),
              pcm.size() / sizeof(int16_t));
      }
  }

  std::lock_guard<std::mutex> lock(_mutex);
  _read_pending = false;
  if (!ok) {
      _closing = true; 
  }
  if (!_closing) {
      _issue_read_l();
  }
  _maybe_finish_l(); 
}

void GRPCAgentStream::_on_write_frame(bool ok) {
//    std::cout << "GRPCAgentStream::_on_write_frame ok "<< ok << std::endl;
  std::lock_guard<std::mutex> lock(_mutex);
  _write_pending = false;
  if (!ok) {
      _closing = true;
  }
  _kick_write_l();
  _maybe_finish_l();
}

void GRPCAgentStream::_on_finish(bool ok) {
    std::cerr << "[stream " << _id << "] finish ok=" << ok
              << " code=" << _status.error_code()
              << " msg=" << _status.error_message() << "\n";
}

void GRPCAgentStream::_issue_read_l() {
    _read_pending = true;
    _stream->Read(&_read_frame, new OpTag(shared_from_this(), OpType::READ));
}

void GRPCAgentStream::_kick_write_l() {
    if (!_started || _closing || _write_pending || _write_queue.empty()) {
        return;
    }

    auto& f = _write_queue.front();
    _write_frame.set_pcm_s16le(
        reinterpret_cast<const char*>(f.data()),
        f.size() * sizeof(int16_t));
    _write_queue.pop_front();

    _write_pending = true;
    _stream->Write(_write_frame, new OpTag(shared_from_this(), OpType::WRITE));
}

void GRPCAgentStream::_maybe_finish_l() {
    if (_closing && !_finish_issued &&
        !_start_pending && !_read_pending && !_write_pending) {
        _finish_issued = true;
        _stream->Finish(&_status, new OpTag(shared_from_this(), OpType::FINISH));
    }
}