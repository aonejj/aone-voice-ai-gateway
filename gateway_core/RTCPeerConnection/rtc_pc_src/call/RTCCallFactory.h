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
 *  call/call_factory.h.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */


#ifndef __RTC_CALL_CALL_FACTORY_H__
#define __RTC_CALL_CALL_FACTORY_H__

//#include "api/task_queue/task_queue_factory.h"
#include "rtc_base/system/no_unique_address.h"

#include "../api/call/RTCCallFactoryInterface.h"

#include "../api/audio/RTCAudioRtpPacketListenSinkInterface.h"
#include "RTCCall.h"


namespace webrtc {

class RTCCallFactory : public RTCCallFactoryInterface {
public:
	RTCCallFactory();

private:
	~RTCCallFactory() override {}

	RTCCall* CreateRTCCall(RTCAudioRtpPacketListenSinkInterface* audio_rtp_packet_listener,
						   RTCVideeRtpPacketListenerProxyGetInterface* video_rtp_packet_listener_proxy_get_interface,
						   rtc::RTCThreadManager *rtc_thread_manager
						   ) override;

};

}

#endif	// __RTC_CALL_CALL_FACTORY_H__