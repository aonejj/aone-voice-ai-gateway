//////////////////////////////////////////////////////////////////////////
//
// author : kimi
// desc   : replace to  /api/call_factory_interface.h
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_API_CALL_CALL_FACTORY_INTERFACE_H__
#define __RTC_API_CALL_CALL_FACTORY_INTERFACE_H__

#include <memory>
#include "../../_deprecate_defines.h"

#include "rtc_base/system/rtc_export.h"

#include "../../../rtc_thread_provider/RTCThreadManager.h"


namespace webrtc {

class RTCCall;

class RTCAudioRtpPacketListenSinkInterface;
class RTCVideeRtpPacketListenerProxyGetInterface;

class RTCCallFactoryInterface {
public:
	virtual ~RTCCallFactoryInterface() {}

	virtual RTCCall* CreateRTCCall(RTCAudioRtpPacketListenSinkInterface* audio_rtp_packet_listener,
								   RTCVideeRtpPacketListenerProxyGetInterface* video_rtp_packet_listener_proxy_get_interface
								   , rtc::RTCThreadManager *rtc_thread_manager
								   ) = 0;
};

RTC_EXPORT std::unique_ptr<RTCCallFactoryInterface> CreateCallFactory();

}	//namespace webrtc

#endif // __RTC_API_CALL_CALL_FACTORY_INTERFACE_H__
