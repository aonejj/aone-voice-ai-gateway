/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/*
 *  Modified by kimi.
 *  This file is a modified replacement for
 *  api/call_factory_interface.h.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */


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
