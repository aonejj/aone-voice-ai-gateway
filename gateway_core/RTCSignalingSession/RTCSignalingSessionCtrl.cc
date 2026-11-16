#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif

#define LOG_TAG "RTCSignalingSessionCtrlImpl"

#include <utils/Log.h>

#include "RTCSignalingSessionImpl.h"
#include "./include/RTCSignalingSessionCtrl.h"


class RTCSignalingSessionCtrl::Impl {
public:
    Impl(RTCSignalingSessionStatusObserver *observer);
    ~Impl() = default;
    
public:
    uint32_t Open(std::string &host, int port, std::string &path);
    uint32_t Close();

    RTCSignalingSessionInterface* GetInterface();
    
private:
    std::shared_ptr<RTCSignalingSessionImpl> _signaling_session;

private:
	Impl(const Impl &) = delete;
	Impl &operator=(const Impl &) = delete;    
};

RTCSignalingSessionCtrl::Impl::Impl(RTCSignalingSessionStatusObserver *observer) {
    _signaling_session = RTCSignalingSessionImpl::Create(observer);
}

uint32_t RTCSignalingSessionCtrl::Impl::Open(std::string &host, int port, std::string &path) {
    status_t sret = _signaling_session->Open(host, port, path);
    if(sret != OK) {
        return -1;
    }

    return 0;
}

uint32_t RTCSignalingSessionCtrl::Impl::Close() {
    status_t sret = _signaling_session->Close();
    if(sret != OK) {
        return -1;
    }

    return 0;    
}

RTCSignalingSessionInterface* RTCSignalingSessionCtrl::Impl::GetInterface() {
    return _signaling_session.get();
}

///////////////////////////////////////////////
std::unique_ptr<RTCSignalingSessionCtrl> RTCSignalingSessionCtrl::Create(RTCSignalingSessionStatusObserver *observer) {
    std::unique_ptr<RTCSignalingSessionCtrl> ptr(new RTCSignalingSessionCtrl(observer));
    
    return std::move(ptr);
}

RTCSignalingSessionCtrl::RTCSignalingSessionCtrl(RTCSignalingSessionStatusObserver *observer) {
    _impl = std::make_unique<RTCSignalingSessionCtrl::Impl>(observer);
}

RTCSignalingSessionCtrl::~RTCSignalingSessionCtrl() {

}

uint32_t RTCSignalingSessionCtrl::Open(std::string &host, int port, std::string &path) {
    if(_impl == nullptr) {
        return -1;
    }

    return _impl->Open(host, port, path);
}

uint32_t RTCSignalingSessionCtrl::Close() {
    if(_impl == nullptr) {
        return -1;
    }

    return _impl->Close();
}

RTCSignalingSessionInterface* RTCSignalingSessionCtrl::GetInterface() {
    return _impl->GetInterface();
}