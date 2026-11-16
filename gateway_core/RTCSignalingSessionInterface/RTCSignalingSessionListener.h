#ifndef __RTC_SIGNALING_SESSION_LISTENER_H__
#define __RTC_SIGNALING_SESSION_LISTENER_H__

#include <string>
#include <vector>

#include "../AgentEndpoint/AgentEndpoint.h"

class RTCSignalingSessionListener {
public:
    RTCSignalingSessionListener() = default;
    ~RTCSignalingSessionListener() = default;

public:
    virtual uint32_t OnConnectedPeer(std::string& id, std::string& agentId) = 0;
    virtual void OnAgentEndPoint(std::vector<AgentEndpoint>& endpoints) = 0;
    virtual void OnAgentRemoved(std::string& id) = 0;
    virtual uint32_t OnDisconnectedPeer(std::string& id) = 0;
    virtual uint32_t OnPeerOffer(std::string& id, 
                                 std::string& sdp) = 0;
    virtual uint32_t OnPeerCandidate(std::string& id, int32_t mline_idx, 
                        std::string& mid, std::string& candidate) = 0;
};    

#endif  // __RTC_SIGNALING_SESSION_LISTENER_H__