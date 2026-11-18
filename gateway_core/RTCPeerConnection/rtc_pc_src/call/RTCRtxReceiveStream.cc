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

#include <string.h>

#include <utility>

#include "api/array_view.h"
#include "modules/rtp_rtcp/include/receive_statistics.h"
#include "modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "modules/rtp_rtcp/source/rtp_packet_received.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"

#include "RTCRtxReceiveStream.h"


namespace webrtc {

RTCRtxReceiveStream::RTCRtxReceiveStream(
	RtpPacketSinkInterface* media_sink,
	std::map<int, int> associated_payload_types,
	uint32_t media_ssrc,
	ReceiveStatistics* rtp_receive_statistics /* = nullptr */)
	: media_sink_(media_sink),
	associated_payload_types_(std::move(associated_payload_types)),
	media_ssrc_(media_ssrc),
	rtp_receive_statistics_(rtp_receive_statistics) {
	if (associated_payload_types_.empty()) {
		RTC_LOG(LS_WARNING)
			<< "RtxReceiveStream created with empty payload type mapping.";
	}
}

RTCRtxReceiveStream::~RTCRtxReceiveStream() = default;

//////////////////////////////////////////////////////////////////////////
// rtx packet dump
static void _rtx_packet_dump(const RtpPacketReceived& rtx_packet) {
	rtc::ArrayView<const uint8_t> payload = rtx_packet.payload();
	RTC_LOG(LS_INFO) << "video-recv-stream-trace RTCRtxReceiveStream::OnRtxRtpPacket rtx ssrc " <<
		rtx_packet.Ssrc() << " seq no " << rtx_packet.SequenceNumber() << " timestamp " << rtx_packet.Timestamp();
	RTC_LOG(LS_INFO) <<	"video-recv-stream-trace RTCRtxReceiveStream::OnRtxRtpPacket payload seq no " <<
		 (payload[0] << 8) + payload[1] << " p[0] " << payload[0] <<" p[1] "<< payload[1];	
}

void RTCRtxReceiveStream::OnRtpPacket(const RtpPacketReceived& rtx_packet) {
	if (rtp_receive_statistics_) {
		rtp_receive_statistics_->OnRtpPacket(rtx_packet);
	}
	rtc::ArrayView<const uint8_t> payload = rtx_packet.payload();

	if (payload.size() < kRtxHeaderSize) {
//		RTC_LOG(LS_INFO) << "[" << this << "] packet_time_trace RTCRtxReceiveStream::OnRtpPacket ssrc " << rtx_packet.Ssrc() <<" seq "<< rtx_packet.SequenceNumber() << " payload size " << payload.size();
		return;
	}

	auto it = associated_payload_types_.find(rtx_packet.PayloadType());
	if (it == associated_payload_types_.end()) {
		RTC_LOG(LS_VERBOSE) << "Unknown payload type "
			<< static_cast<int>(rtx_packet.PayloadType())
			<< " on rtx ssrc " << rtx_packet.Ssrc();
		return;
	}

	media_sink_->OnRtxRtpPacket(rtx_packet);
}

}