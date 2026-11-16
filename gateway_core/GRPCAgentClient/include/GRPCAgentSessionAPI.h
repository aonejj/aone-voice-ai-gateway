#ifndef __GRPC_AGENT_SESSION_API_H__
#define __GRPC_AGENT_SESSION_API_H__

#include <memory>
#include <string>

#include "../../RTCAgentBridge/AgentEgressAudioSinkInterface.h"
#include "../../RTCAudioFormat/RTCAudioFormat.h"

#if defined(__GNUC__) || defined(__clang__)
#define GRPC_AGENT_EXPORT __attribute__((visibility("default")))
#else
#define GRPC_AGENT_EXPORT
#endif

class GRPCAgentSession {
public:
    virtual ~GRPCAgentSession() = default;

public:
    virtual int32_t Open() = 0;
    virtual void Close() = 0;
    virtual RTCAudioFormat GetAgentSessionInAudioFormat() = 0;
    virtual RTCAudioFormat GetAgentSessionOutAudioFormat() = 0;
    virtual void SetSink(AgentEgressAudioSinkInterface* sink) = 0;
    virtual void SendAudioFrame(const uint8_t *pcm, size_t size) = 0;
    virtual void Release() = 0;
};

GRPC_AGENT_EXPORT void GRPCAgentSessionInitialize();
GRPC_AGENT_EXPORT int32_t GRPCAddAgentEndPoint(const std::string& endpoint);
GRPC_AGENT_EXPORT void GRPCRemoveAgentEndPoint(const std::string& endpoint);
GRPC_AGENT_EXPORT std::shared_ptr<GRPCAgentSession> CreateGRPCAgentSession(const std::string& endpoint, 
    const std::string& id, std::string* error);

#endif // __GRPC_AGENT_SESSION_API_H__