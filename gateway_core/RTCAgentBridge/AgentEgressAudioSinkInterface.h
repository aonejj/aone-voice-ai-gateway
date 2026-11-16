#ifndef __AGENT_EGRESS_AUDIO_SINK_INTERFACE_H__
#define __AGENT_EGRESS_AUDIO_SINK_INTERFACE_H__

class AgentEgressAudioSinkInterface {
public:
    AgentEgressAudioSinkInterface() = default;
    ~AgentEgressAudioSinkInterface() = default;
    
public:
    virtual bool PushPcm(const int16_t* pcm, size_t samples) = 0;
    virtual void Clear() = 0;
    virtual uint32_t QueuedMs() const = 0;    
};

#endif // __AGENT_EGRESS_AUDIO_SINK_INTERFACE_H__