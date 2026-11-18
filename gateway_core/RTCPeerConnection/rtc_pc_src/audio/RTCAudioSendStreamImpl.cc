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
 *  audio/audio_send_stream.h.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#include "rtc_base/event.h"
#include "rtc_base/logging.h"
//#include "api/task_queue/task_queue_factory.h"

#include "RTCAudioPacketRouteFeeder.h"
#include "RTCAudioSendStreamImpl.h"
#include "../call/RTCRtpTransportControllerSendInterface.h"

namespace webrtc {

RTCAudioSendStreamImpl::RTCAudioSendStreamImpl(Clock* clock,
	RTCAudioSendStream::Config config,
	RTCRtpTransportControllerSendInterface* rtp_transport,
	RtcpRttStats* rtt_stats)
	:
	worker_queue_(TaskQueueBase::Current()),
	config_(std::move(config)) {
	RTC_LOG(LS_VERBOSE) << "[" << this << "] RTCAudioSendStreamImpl config "<< config.ToString();

	feeder_ = std::make_unique<RTCAudioPacketRouteFeeder>(clock, &config_);

	router_.reset(new RTCAudioSendStreamRouter(clock,
		&config_,
		feeder_.get(),
		rtp_transport,
		rtt_stats
		));
}

RTCAudioSendStreamImpl::~RTCAudioSendStreamImpl() {
	RTC_LOG(LS_VERBOSE) << "["<<this<<"] ~RTCAudioSendStreamImpl";
	RTC_DCHECK_RUN_ON(&thread_checker_);
	RTC_DCHECK(!router_);
}

void RTCAudioSendStreamImpl::DeliverRtcp(const uint8_t* packet, size_t length) {
	// Called on a network thread.
	router_->DeliverRtcp(packet, length);
}

void RTCAudioSendStreamImpl::Start() {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	RTC_LOG(LS_INFO) << "["<<this<<"] RTCAudioSendStreamImpl::Start";

	router_->Start();
}

void RTCAudioSendStreamImpl::Stop() {
	RTC_DCHECK_RUN_ON(&thread_checker_);
//	RTC_LOG(LS_INFO) << "["<<this<<"] RTCAudioSendStreamImpl::Stop";

	router_->Stop();
}

const RTCAudioSendStream::Config& RTCAudioSendStreamImpl::GetConfig() const {
	return config_;
}

void RTCAudioSendStreamImpl::SetSource(
	rtc::RTCAudioPacketRouteSourceInterface<std::unique_ptr<webrtc::RtpPacketReceived>>* source) {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	feeder_->SetSource(source);
}

RTCAudioSendStream::Stats RTCAudioSendStreamImpl::GetStats() const {
	RTCAudioSendStream::Stats stats;
	return stats;
}

void RTCAudioSendStreamImpl::StopPermanently() {
	RTC_LOG(LS_INFO) << "[" << this << "] RTCAudioSendStreamImpl::StopPermanently";

	if (TaskQueueBase::Current() != worker_queue_) {
		worker_queue_->PostTask(SafeTask(safety_.flag(), [this]() {
			RTC_DCHECK_RUN_ON(&thread_checker_);
			router_->Stop();
			router_.reset();
			thread_sync_event_.Set();
		}));
		thread_sync_event_.Wait(rtc::Event::kForever);
		return;
	}
	RTC_DCHECK_RUN_ON(&thread_checker_);
	router_->Stop();
	router_.reset();
}

}	// namespace webrtc