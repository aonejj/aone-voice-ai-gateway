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
 *  module/pacing/task_queue_paced_sender.h.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#ifndef __RTC_MODULE_PACING_TASK_QUEUE_PACED_SENDER__
#define __RTC_MODULE_PACING_TASK_QUEUE_PACED_SENDER__

#include <unordered_map>

#include <pthread.h>

#include "absl/types/optional.h"

#include "api/units/data_rate.h"
#include "api/units/time_delta.h"
#include "api/units/timestamp.h"
#include "modules/rtp_rtcp/include/rtp_packet_sender.h"
#include "modules/pacing/rtp_packet_pacer.h"
#include "modules/pacing/bitrate_prober.h"
#include "rtc_base/numerics/exp_filter.h"
#include "rtc_base/task_utils/pending_task_safety_flag.h"

#include "../rtp_rtcp/source/RTCRtpRtcpInterface.h"
#include "RTCSFUPacingListener.h"
#include "RTCPacingController.h"
#include "RTCSFUPacingController.h"
#include "../../RTCUtils/RTCSectionProfiler.h"

#include "../rtc_pc_src/_deprecate_defines.h"

namespace webrtc {

class Clock;

class RTCTaskQueuePacedSender : public RtpPacketPacer, public RtpPacketSender
{
public:
	static const int kNoPacketHoldback;

	RTCTaskQueuePacedSender(
		Clock* clock,
		RTCPacingController::RTCPacketSender* packet_sender,
		TimeDelta max_hold_back_window,
		int max_hold_back_window_in_packets,
		absl::optional<TimeDelta> burst_interval = absl::nullopt);

	~RTCTaskQueuePacedSender() override;

	void EnsureStarted();

	// RtpPacketSender method
	void EnqueuePackets(
		std::vector<std::unique_ptr<RtpPacketToSend>> packets) override;
	void EnqueuePacket(std::unique_ptr<RtpPacketToSend> packet) override;
	void RemovePacketsForSsrc(uint32_t ssrc) override;

	// RtpPacketPacer
	void CreateProbeClusters(
		std::vector<ProbeClusterConfig> probe_cluster_configs) override;
	void Pause() override;
	void Resume() override;
	void SetCongested(bool congested) override;
	void SetPacingRates(DataRate pacing_rate, DataRate padding_rate) override;
	void SetAccountForAudioPackets(bool account_for_audio) override;
	void SetIncludeOverhead() override;
	void SetTransportOverhead(DataSize overhead_per_packet) override;
	TimeDelta OldestPacketWaitTime() const override;
	DataSize QueueSizeData() const override;
	absl::optional<Timestamp> FirstSentPacketTime() const override;
	TimeDelta ExpectedQueueTime() const override;
	void SetQueueTimeLimit(TimeDelta limit) override;

protected:
	struct Stats {
		Stats()
			: oldest_packet_enqueue_time(Timestamp::MinusInfinity()),
			queue_size(DataSize::Zero()),
			expected_queue_time(TimeDelta::Zero()) {}
		Timestamp oldest_packet_enqueue_time;
		DataSize queue_size;
		TimeDelta expected_queue_time;
		absl::optional<Timestamp> first_sent_packet_time;
	};
	void OnStatsUpdated(const Stats& stats);

private:
	void MaybeScheduleProcessPackets() RTC_RUN_ON(task_queue_);
	void MaybeProcessPackets(Timestamp scheduled_process_time);
	void UpdateStats() RTC_RUN_ON(task_queue_);
	Stats GetStats() const;

private:
	Clock* const clock_;
	const TimeDelta max_hold_back_window_;
	const int max_hold_back_window_in_packets_;
	RTCPacingController pacing_controller_ RTC_GUARDED_BY(task_queue_);

	Timestamp next_process_time_ RTC_GUARDED_BY(task_queue_);
	bool is_started_ RTC_GUARDED_BY(task_queue_);
	bool is_shutdown_ RTC_GUARDED_BY(task_queue_);

	rtc::ExpFilter packet_size_ RTC_GUARDED_BY(task_queue_);
	bool include_overhead_ RTC_GUARDED_BY(task_queue_);

	Stats current_stats_ RTC_GUARDED_BY(task_queue_);
	bool processing_packets_ RTC_GUARDED_BY(task_queue_) = false;

	ScopedTaskSafety safety_;
	TaskQueueBase* task_queue_;

	absl::optional<TimeDelta> burst_interval_;

#ifdef __PERFORMANCE_PROFILE_ENABLED__
	RTCSectionStat _prof_post_enqueue{"post_enqueue"};
#endif 
};

}


#endif // __RTC_MODULE_PACING_TASK_QUEUE_PACED_SENDER__