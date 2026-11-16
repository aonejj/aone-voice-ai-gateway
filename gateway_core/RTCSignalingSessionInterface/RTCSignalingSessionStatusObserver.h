#ifndef __RTC_SIGNALING_SESSION_STATUS_OBSERVER_INTERFACE_H__
#define __RTC_SIGNALING_SESSION_STATUS_OBSERVER_INTERFACE_H__

class RTCSignalingSessionStatusObserver {
public:
    RTCSignalingSessionStatusObserver() = default;
    ~RTCSignalingSessionStatusObserver() = default;

public:
	virtual uint32_t OnSignalingSessionOpened() = 0;
	virtual uint32_t OnSignalingSessionClosed() = 0;
	virtual uint32_t OnSignalingSessionError() = 0;         
};   

#endif  // __RTC_SIGNALING_SESSION_STATUS_OBSERVER_INTERFACE_H__