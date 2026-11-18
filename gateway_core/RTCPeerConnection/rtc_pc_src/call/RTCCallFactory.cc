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
 *  call/call_factory.cc.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#include "RTCCallFactory.h"

namespace webrtc {

RTCCallFactory::RTCCallFactory() {

}

RTCCall* RTCCallFactory::CreateRTCCall(RTCAudioRtpPacketListenSinkInterface* audio_rtp_packet_listener,
	RTCVideeRtpPacketListenerProxyGetInterface* video_rtp_packet_listener_proxy_get_interface,
	rtc::RTCThreadManager *rtc_thread_manager
	) {

	return RTCCall::Create(
		audio_rtp_packet_listener, 
		video_rtp_packet_listener_proxy_get_interface,
		rtc_thread_manager
		);
}

std::unique_ptr<RTCCallFactoryInterface> CreateCallFactory() {
	return std::unique_ptr<RTCCallFactoryInterface>(new RTCCallFactory());
}

} // namespace webrtc