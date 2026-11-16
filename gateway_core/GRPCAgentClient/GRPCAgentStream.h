#ifndef __GRPC_AGENT_STREAM_H__
#define __GRPC_AGENT_STREAM_H__

#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include <queue>

#include <grpcpp/grpcpp.h>

#include "voice_agent.grpc.pb.h"
#include "voice_agent.pb.h"
#include "GRPCCQTag.h"


class GRPCChannelInterface;
class AgentEgressAudioSinkInterface;

class GRPCAgentStream : public std::enable_shared_from_this<GRPCAgentStream> {
public:
    GRPCAgentStream(const std::string& id, GRPCChannelInterface* channel);
    ~GRPCAgentStream();

public:
    int32_t CreateStream();
    void Release();
    void SendAudioFrame(const uint8_t *pcm, size_t size);
    void SetSink(AgentEgressAudioSinkInterface* sink);

private:
    enum class OpType { START, READ, WRITE, FINISH };
    
private:
    class OpTag : public GRPCCQTag {
    public:
        OpTag (std::shared_ptr<GRPCAgentStream> stream, OpType op)
        : _stream(stream),
          _op(op) { }

        void Proceed(bool ok) override {
            _stream->OnComplete(_op, ok);
        }
    
    private:
        std::shared_ptr<GRPCAgentStream> _stream;
        OpType _op;     
    };    

private:
    void OnComplete(OpType op, bool ok);    
    void _on_start(bool ok);
    void _on_read_frame(bool ok);
    void _on_write_frame(bool ok);
    void _on_finish(bool ok);

    void _issue_read_l();
    void _kick_write_l();
    void _maybe_finish_l();
    
private:
    static constexpr size_t kMaxWriteQueue = 50;    

private:
    std::string _id;
    GRPCChannelInterface* _channel_ptr;
    grpc::ClientContext _context;
    std::shared_ptr<grpc::ClientAsyncReaderWriter<
            voice::agent::v1::AudioFrame,
            voice::agent::v1::AudioFrame>> _stream;
    grpc::Status _status;         
    
    std::mutex _mutex;
    bool _started = false;
    bool _closing = false;
    bool _finish_issued = false;
    bool _start_pending = false;
    bool _read_pending = false;
    bool _write_pending = false;
    std::deque<std::vector<int16_t>> _write_queue;
    voice::agent::v1::AudioFrame _write_frame;

    std::mutex _sink_mutex;
    AgentEgressAudioSinkInterface* _sink = nullptr;
    voice::agent::v1::AudioFrame _read_frame;

private:
    GRPCAgentStream(const GRPCAgentStream&) = delete;
    GRPCAgentStream& operator=(const GRPCAgentStream&) = delete;      
};

#endif // __GRPC_AGENT_STREAM_H__