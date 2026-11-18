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
 *  call/rtx_receive_stream.h.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#ifndef __RTC_CALL_RTX_RECEIVE_STREAM_H__
#define __RTC_CALL_RTX_RECEIVE_STREAM_H__

#include <cstdint>
#include <map>

#include "call/rtp_packet_sink_interface.h"

namespace webrtc {

class ReceiveStatistics;

class RTCRtxReceiveStream : public RtpPacketSinkInterface {
public:
	RTCRtxReceiveStream(RtpPacketSinkInterface* media_sink,
		std::map<int, int> associated_payload_types,
		uint32_t media_ssrc,
		ReceiveStatistics* rtp_receive_statistics = nullptr);
	~RTCRtxReceiveStream() override;
	// RtpPacketSinkInterface.
	void OnRtpPacket(const RtpPacketReceived& packet) override;

private:
	RtpPacketSinkInterface* const media_sink_;
	const std::map<int, int> associated_payload_types_;
	const uint32_t media_ssrc_;
	ReceiveStatistics* const rtp_receive_statistics_;
};

}

#endif // __RTC_CALL_RTX_RECEIVE_STREAM_H__