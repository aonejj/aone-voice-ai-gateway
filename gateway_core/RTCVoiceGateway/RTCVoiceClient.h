#ifndef __RTC_VOICE_CLIENT_H__
#define __RTC_VOICE_CLIENT_H__

#include <memory>
#include <string>

#include <utils/Errors.h>

class RTCClientInterface;
class AgentClientInterface;

class RTCVoiceClient {
public:
    RTCVoiceClient(const std::string& id, std::shared_ptr<RTCClientInterface> peer,
                    std::shared_ptr<AgentClientInterface> agent);
    ~RTCVoiceClient();

public:
    RTCClientInterface* GetPeerPtr();
    AgentClientInterface* GetAgentPtr();
    status_t Open();
    status_t Close();

private:
    std::string _id;
    std::shared_ptr<RTCClientInterface> _peer;
    std::shared_ptr<AgentClientInterface> _agent;

private:
    RTCVoiceClient(const RTCVoiceClient&) = delete;
    RTCVoiceClient& operator=(const RTCVoiceClient&) = delete;
};

#endif // __RTC_VOICE_CLIENT_H__