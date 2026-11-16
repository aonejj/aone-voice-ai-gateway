#ifndef __GRPC_AGENT_ENDPOINT_H__
#define __GRPC_AGENT_ENDPOINT_H__

#include <memory>
#include <string>
#include <chrono>
#include <vector>

#include "voice_agent.grpc.pb.h"
#include "GRPCChannelInterface.h"


struct AgentAudioFormats {
    std::vector<voice::agent::v1::AudioFormat> ingress;
    std::vector<voice::agent::v1::AudioFormat> egress;
};

class GRPCAgentStream;
class GRPCCQInterface;

class GRPCAgentChannel : public GRPCChannelInterface {
public:
    GRPCAgentChannel(const std::string& endpoint, GRPCCQInterface* cq);
    ~GRPCAgentChannel();
    
public:
    void* GetStubPtr() override;
    void* GetCQPtr() override;

    int32_t CreateChannel() override;    
    std::string GetEndpoint() const override;
    int32_t ConnectAndHandshake(std::chrono::milliseconds total_timeout) override; 

    RTCAudioFormat GetAgentInAudioFormat() override;
    RTCAudioFormat GetAgentOutAudioFormat() override;  
    
    void CloseChannel();

private:
    grpc::Status _query_audio_formats(std::chrono::milliseconds timeout);

private:
    std::string _endpoint;
    GRPCCQInterface* _cq;
    std::shared_ptr<grpc::Channel> _channel;
    std::unique_ptr<voice::agent::v1::VoiceAgentService::Stub> _stub;
    AgentAudioFormats _agent_audio_formats;

private:
    GRPCAgentChannel(const GRPCAgentChannel&) = delete;
    GRPCAgentChannel& operator=(const GRPCAgentChannel&) = delete;      
};

#endif // __GRPC_AGENT_ENDPOINT_H__