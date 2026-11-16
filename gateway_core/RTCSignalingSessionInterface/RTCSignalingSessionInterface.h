#ifndef __RTC_SIGNALING_SESSION_INTERFACE_H__
#define __RTC_SIGNALING_SESSION_INTERFACE_H__

#include <string>
#include <vector>

#include "../RTCIceInfo/RTCIceInfo.h"
#include "../AgentEndpoint/AgentEndpoint.h"

class RTCSignalingSessionListener;

class RTCSignalingSessionInterface {
public:
    RTCSignalingSessionInterface() = default;
    ~RTCSignalingSessionInterface() = default;

public:
    virtual void SetListener(RTCSignalingSessionListener* listener) = 0;
    virtual std::vector<RTCIceServerInfo> GetIceServerInfos() = 0;
    virtual std::vector<AgentEndpoint> GetAgentEndpoint() = 0;
    virtual uint32_t SendAnswer(const std::string& id, const std::string& sdp) = 0;
    virtual uint32_t SendCandidate(const std::string& id, int32_t index_line,
         const std::string& mid, const std::string& candidate) = 0;
    virtual uint32_t SendIceState(const std::string& id, int32_t state, 
        const std::string& desc) = 0;
};    


#endif // __RTC_SIGNALING_SESSION_INTERFACE_H__