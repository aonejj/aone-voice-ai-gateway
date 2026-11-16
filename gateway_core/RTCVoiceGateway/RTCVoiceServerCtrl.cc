#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif

#define LOG_TAG "RTCVoiceServerCtrlImpl"

#include <utils/Log.h>

#include "RTCVoiceServer.h"
#include "./include/RTCVoiceServerCtrl.h"

class RTCVoiceServerCtrl::Impl {
public:
    Impl(RTCSignalingSessionInterface* interface);
    ~Impl() = default;
    
public:
    uint32_t Start();
    void Stop();
    
private:
    std::shared_ptr<RTCVoiceServer> _voice_server;

private:
	Impl(const Impl &) = delete;
	Impl &operator=(const Impl &) = delete;    
};

RTCVoiceServerCtrl::Impl::Impl(RTCSignalingSessionInterface* interface) {
    _voice_server = RTCVoiceServer::Create(interface);
}

uint32_t RTCVoiceServerCtrl::Impl::Start() {
    return _voice_server->Start();
}

void RTCVoiceServerCtrl::Impl::Stop() {
    _voice_server->Stop();
}

//////////////////////////////////////////////////////////////

std::unique_ptr<RTCVoiceServerCtrl> RTCVoiceServerCtrl::Create(RTCSignalingSessionInterface* interface) {
    std::unique_ptr<RTCVoiceServerCtrl> ptr(new RTCVoiceServerCtrl(interface));
    
    return std::move(ptr);
}

RTCVoiceServerCtrl::RTCVoiceServerCtrl(RTCSignalingSessionInterface* interface) {
    _state = RTC_VOICE_SERVER_STATE_INIT;
    _impl = std::make_unique<RTCVoiceServerCtrl::Impl>(interface);
}

RTCVoiceServerCtrl::~RTCVoiceServerCtrl() {
    if(_state == RTC_VOICE_SERVER_STATE_STARTED) {
        Stop();
    }
}

uint32_t RTCVoiceServerCtrl::Start() {
    if(_impl == nullptr) {
        return -1;
    }

    if(_state == RTC_VOICE_SERVER_STATE_STARTED) {
        return 0;
    }

    uint32_t ret = _impl->Start();
    if(ret == 0) {
        _state = RTC_VOICE_SERVER_STATE_STARTED;
    } else {
        _state = RTC_VOICE_SERVER_STATE_ERROR;
    }

    return ret;
}

void RTCVoiceServerCtrl::Stop() {
    if(_impl == nullptr) {
        return;
    }

    if(_state == RTC_VOICE_SERVER_STATE_STOPPED || 
        _state == RTC_VOICE_SERVER_STATE_INIT) {
        return;
    }    

    _impl->Stop();
    _state = RTC_VOICE_SERVER_STATE_STOPPED;

}
