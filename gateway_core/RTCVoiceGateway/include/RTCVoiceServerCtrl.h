#include <memory>
#include <string>

#include "RTCSignalingSessionInterface.h"

class RTCVoiceServerCtrl {
public:
    enum RTCVoiceServerState {
        RTC_VOICE_SERVER_STATE_INIT,
        RTC_VOICE_SERVER_STATE_STARTED,
        RTC_VOICE_SERVER_STATE_STOPPED,
        RTC_VOICE_SERVER_STATE_ERROR,
    };

public:
    static std::unique_ptr<RTCVoiceServerCtrl> Create(RTCSignalingSessionInterface* interface);
    ~RTCVoiceServerCtrl();

public:
    uint32_t Start();
    void Stop();    

private:
    RTCVoiceServerCtrl(RTCSignalingSessionInterface* interface); 

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
    RTCVoiceServerState _state;
    
private:
	RTCVoiceServerCtrl(const RTCVoiceServerCtrl &) = delete;
	RTCVoiceServerCtrl &operator=(const RTCVoiceServerCtrl &) = delete;    

};