#include "GRPCAgentSessionAPI.h"

#include "GRPCChannelInterface.h"
#include "GRPCAgentChannel.h"
#include "GRPCAgentChannelManager.h"
#include "GRPCAgentStream.h"

////////////////////////////////////////////////////////
// GRPCAgentSessionImpl
class GRPCAgentSessionImpl : public GRPCAgentSession {
public:
    GRPCAgentSessionImpl(const std::string& endpoint, const std::string& id);
    ~GRPCAgentSessionImpl();

public:
    int32_t Open() override;
    void Close() override;
    RTCAudioFormat GetAgentSessionInAudioFormat() override;
    RTCAudioFormat GetAgentSessionOutAudioFormat() override;
    void SetSink(AgentEgressAudioSinkInterface* sink) override;
    void SendAudioFrame(const uint8_t *pcm, size_t size) override;
    void Release() override;        

private:
    std::string _endpoint;
    std::string _id;

    RTCAudioFormat _agent_in_format;
    RTCAudioFormat _agent_out_format;    
    std::shared_ptr<GRPCAgentStream> _stream;
private:
    GRPCAgentSessionImpl(const GRPCAgentSessionImpl&) = delete;
    GRPCAgentSessionImpl& operator=(const GRPCAgentSessionImpl&) = delete;    
};

GRPCAgentSessionImpl::GRPCAgentSessionImpl(const std::string& endpoint, const std::string& id)
 : _endpoint(endpoint),
   _id(id) {
    
}

GRPCAgentSessionImpl::~GRPCAgentSessionImpl() {

}

int32_t GRPCAgentSessionImpl::Open() {
    std::shared_ptr<GRPCChannelInterface> channel = GRPCAgentChannelManager::GetInstance().FindChannel(_endpoint);
    if(channel == nullptr) {
        return -1;
    }

    _agent_in_format = channel->GetAgentInAudioFormat();
    _agent_out_format = channel->GetAgentOutAudioFormat();
    
    _stream = std::make_shared<GRPCAgentStream>(_id, channel.get());
    if(_stream == nullptr) {
        return -1;
    }

    if(_stream->CreateStream() != 0) {
        _stream.reset();
        return -1;
    }

    return 0;
}

void GRPCAgentSessionImpl::Close() {

}

RTCAudioFormat GRPCAgentSessionImpl::GetAgentSessionInAudioFormat() {
    return _agent_in_format;
}

RTCAudioFormat GRPCAgentSessionImpl::GetAgentSessionOutAudioFormat() {
    return _agent_out_format;
}

void GRPCAgentSessionImpl::SetSink(AgentEgressAudioSinkInterface* sink) {
    if(_stream != nullptr) {
        _stream->SetSink(sink);
    }
}

void GRPCAgentSessionImpl::SendAudioFrame(const uint8_t *pcm, size_t size) {
    if(_stream != nullptr) {
        _stream->SendAudioFrame(pcm, size);
    }
}

void GRPCAgentSessionImpl::Release() {
    if(_stream != nullptr) {
        _stream->Release();
    }
}

void GRPCAgentSessionInitialize() {
    GRPCAgentChannelManager::GetInstance().CreateCQ();
}

int32_t GRPCAddAgentEndPoint(const std::string& endpoint) {
    std::shared_ptr<GRPCChannelInterface> channel = GRPCAgentChannelManager::GetInstance().FindChannel(endpoint);
    if(channel != nullptr) {
        return 0;
    }

    channel = std::make_shared<GRPCAgentChannel>(endpoint, GRPCAgentChannelManager::GetInstance().GetCQInterface());
    if(channel == nullptr) {
        return -1;
    }

    int32_t ret = channel->CreateChannel();
    if(ret) {
        return ret;
    }

    ret = channel->ConnectAndHandshake(std::chrono::seconds(10));
    if(ret) {
        return ret;
    }
    
    GRPCAgentChannelManager::GetInstance().AddChannel(channel);
    return 0;    
}

void GRPCRemoveAgentEndPoint(const std::string& endpoint) {
    GRPCAgentChannelManager::GetInstance().RemoveChannel(endpoint);
}

std::shared_ptr<GRPCAgentSession> CreateGRPCAgentSession(const std::string& endpoint, 
    const std::string& id, std::string* error) {
    if (endpoint.empty() || id.empty()) {
        return nullptr;
    }

    return std::make_shared<GRPCAgentSessionImpl>(endpoint, id);
}
