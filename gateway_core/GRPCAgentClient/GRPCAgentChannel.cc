
#include <chrono>
#include <algorithm> 
#include <thread>
#include <grpcpp/grpcpp.h>
#include <iostream>

#include "GRPCCQInterface.h"
#include "GRPCAgentChannel.h"

GRPCAgentChannel::GRPCAgentChannel(const std::string& endpoint,  GRPCCQInterface* cq)
 : _endpoint(endpoint),
   _cq(cq) {

 }

 GRPCAgentChannel::~GRPCAgentChannel() {
    CloseChannel();
 }

int32_t GRPCAgentChannel::CreateChannel() {
    if(_endpoint.empty()) {
        return -1;
    }

    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 10000);          
    args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 3000); 
    args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);   

    _channel = grpc::CreateCustomChannel(_endpoint, grpc::InsecureChannelCredentials(), args);
    _stub = voice::agent::v1::VoiceAgentService::NewStub(_channel);

    return 0;
}

void* GRPCAgentChannel::GetStubPtr() {
    return (void*)_stub.get();
}

void* GRPCAgentChannel::GetCQPtr() {
    return _cq->GetCQPtr();
}

std::string GRPCAgentChannel::GetEndpoint() const {
    return _endpoint;
}

int32_t GRPCAgentChannel::ConnectAndHandshake(std::chrono::milliseconds total_timeout) {
    using clock = std::chrono::steady_clock;
    const auto end = clock::now() + total_timeout;

    std::chrono::milliseconds backoff(200);
    const std::chrono::milliseconds kMaxBackoff(2000);
    grpc::Status last(grpc::StatusCode::DEADLINE_EXCEEDED, "handshake timeout");

    while (true) {
        auto remain = std::chrono::duration_cast<std::chrono::milliseconds>(end - clock::now());
        if (remain.count() <= 0) {
            return last.error_code();
        }

        auto attempt = std::min(remain, std::chrono::milliseconds(2000));
        last = _query_audio_formats(attempt);
        if (last.ok()) {
            return last.error_code();
        }

        switch (last.error_code()) {
        case grpc::StatusCode::UNIMPLEMENTED:
        case grpc::StatusCode::INVALID_ARGUMENT:
        case grpc::StatusCode::PERMISSION_DENIED:
        case grpc::StatusCode::UNAUTHENTICATED:
        case grpc::StatusCode::FAILED_PRECONDITION:
            return last.error_code();
        default:
            break;
        }

        remain = std::chrono::duration_cast<std::chrono::milliseconds>(end - clock::now());
        if (remain.count() <= 0) {
            return last.error_code();
        }
        std::this_thread::sleep_for(std::min(backoff, remain));
        backoff = std::min(backoff * 2, kMaxBackoff);
    }
}

RTCAudioFormat GRPCAgentChannel::GetAgentInAudioFormat() {
    RTCAudioFormat format;
    format.codec = RTCAudioFormat::Codec::kPcmS16le;
    format.sample_rate = _agent_audio_formats.ingress[0].sample_rate_hz();
    format.channels = _agent_audio_formats.ingress[0].channels();

    if(!_agent_audio_formats.ingress[0].is_encoding()) {
        format.codec = RTCAudioFormat::Codec::kPcmS16le;
    }
    return format;
}

RTCAudioFormat GRPCAgentChannel::GetAgentOutAudioFormat() {
    RTCAudioFormat format;
    format.codec = RTCAudioFormat::Codec::kPcmS16le;
    format.sample_rate = _agent_audio_formats.egress[0].sample_rate_hz();
    format.channels = _agent_audio_formats.egress[0].channels();

    if(!_agent_audio_formats.egress[0].is_encoding()) {
        format.codec = RTCAudioFormat::Codec::kPcmS16le;
    }
    return format;
}

void GRPCAgentChannel::CloseChannel() {
    std::cout<<" GRPCAgentChannel::CloseChannel "<< std::endl;
    
    if(_stub != nullptr) {
        _stub.reset();
    }

    if(_channel != nullptr) {
        _channel.reset();
    }
}

grpc::Status GRPCAgentChannel::_query_audio_formats(std::chrono::milliseconds timeout) {
    if (!_stub) {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                            "channel not created");
    }

    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() + timeout);
    ctx.set_wait_for_ready(true);

    voice::agent::v1::AudioFormatRequest req;
    voice::agent::v1::AudioFormatReply rep;

    grpc::Status st = _stub->GetAudioFormats(&ctx, req, &rep);
    if (!st.ok()) {
        return st;
    }

    _agent_audio_formats.ingress.assign(rep.ingress_formats().begin(), rep.ingress_formats().end());
    _agent_audio_formats.egress.assign(rep.egress_formats().begin(), rep.egress_formats().end());

    if (_agent_audio_formats.ingress.empty() || _agent_audio_formats.egress.empty()) {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                            "agent returned no audio formats");
    }

    std::cout<<"ingress format sampling rate " << _agent_audio_formats.ingress[0].sample_rate_hz() <<
    " channels " <<_agent_audio_formats.ingress[0].channels() << std::endl;

    return grpc::Status::OK;
}