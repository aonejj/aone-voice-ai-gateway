#ifndef __AGENT_CLIENT_INTERFACE_H__
#define __AGENT_CLIENT_INTERFACE_H__

#include "../RTCClient/RTCIngressAudioPacketSinkInterface.h"
#include "../RTCClient/RTCEgressAudioSourceInterface.h"

class AgentClientInterface {
public:
    ~AgentClientInterface() = default;

public:
    virtual int32_t Open() = 0;
    virtual void Close() = 0;
    virtual RTCIngressAudioPacketSinkInterface* GetIngressPtr() = 0;
    virtual RTCEgressAudioSourceInterface* GetEgressPtr() = 0;    
};

#endif // __AGENT_CLIENT_INTERFACE_H__