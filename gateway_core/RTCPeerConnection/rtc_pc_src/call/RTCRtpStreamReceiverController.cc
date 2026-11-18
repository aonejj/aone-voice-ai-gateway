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
 *  call/rtp_stream_receiver_controller.cc.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#include "rtc_base/logging.h"
#include "modules/rtp_rtcp/source/rtp_packet_received.h"

#include "RTCRtpStreamReceiverController.h"

namespace webrtc {

RTCRtpStreamReceiverController::RTCReceiver::RTCReceiver(
	RTCRtpStreamReceiverController* controller,
	uint32_t ssrc,
	RtpPacketSinkInterface* sink)
	: controller_(controller), sink_(sink) {
	const bool sink_added = controller_->AddSink(ssrc, sink_);		
	if (!sink_added) {
		RTC_LOG(LS_ERROR)
			<< "RTCRtpStreamReceiverController::RTCReceiver::RTCReceiver: Sink "
			"could not be added for SSRC="
			<< ssrc << ".";
	}
}

RTCRtpStreamReceiverController::RTCReceiver::~RTCReceiver() {
	controller_->RemoveSink(sink_);
}

RTCRtpStreamReceiverController::RTCRtpStreamReceiverController() {}

RTCRtpStreamReceiverController::~RTCRtpStreamReceiverController() = default;

std::unique_ptr<RtpStreamReceiverInterface>
RTCRtpStreamReceiverController::CreateReceiver(uint32_t ssrc,
	RtpPacketSinkInterface* sink) {
	return std::make_unique<RTCReceiver>(this, ssrc, sink);
}

bool RTCRtpStreamReceiverController::OnRtpPacket(const RtpPacketReceived& packet) {
	MutexLock lock(&demuxer_mutex_);
	return demuxer_.OnRtpPacket(packet);
}

bool RTCRtpStreamReceiverController::AddSink(uint32_t ssrc,
		RtpPacketSinkInterface* sink) {
	MutexLock lock(&demuxer_mutex_);
	return demuxer_.AddSink(ssrc, sink);
}

size_t RTCRtpStreamReceiverController::RemoveSink(
		const RtpPacketSinkInterface* sink) {
	MutexLock lock(&demuxer_mutex_);
	return demuxer_.RemoveSink(sink);
}

}