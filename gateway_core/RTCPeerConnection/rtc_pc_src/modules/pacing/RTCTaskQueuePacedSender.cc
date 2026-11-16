//////////////////////////////////////////////////////////////////////////
//
// author : kimi
// desc   : replace to  /module/pacing/task_queue_paced_sender.cc
//
//////////////////////////////////////////////////////////////////////////

#include "absl/cleanup/cleanup.h"
#include "api/transport/network_types.h"
#include "rtc_base/checks.h"
#include "rtc_base/time_utils.h"

#include "RTCTaskQueuePacedSender.h"

#include <pthread.h>

#include "../rtc_pc_src/RTCFunctionTracer.h"

namespace webrtc {

const int RTCTaskQueuePacedSender::kNoPacketHoldback = -1;

RTCTaskQueuePacedSender::RTCTaskQueuePacedSender(
	Clock* clock,
	RTCPacingController::RTCPacketSender* packet_sender,
	TimeDelta max_hold_back_window,
	int max_hold_back_window_in_packets,
	absl::optional<TimeDelta> burst_interval /*= absl::nullopt*/) 
	: clock_(clock),
	  max_hold_back_window_(max_hold_back_window),
	  max_hold_back_window_in_packets_(max_hold_back_window_in_packets), 
	  pacing_controller_(clock, packet_sender),
	  next_process_time_(Timestamp::MinusInfinity()),
	  is_started_(false), 
	  is_shutdown_(false), 
	  packet_size_(/*alpha=*/0.95),		// thinking....
	  include_overhead_(false), 
	  task_queue_(TaskQueueBase::Current()) {
//	RTC_CLASS_FUNCTION_TRACER("RTCTaskQueuePacedSender::RTCTaskQueuePacedSender", this);
	RTC_DCHECK_GE(max_hold_back_window_, RTCPacingController::kMinSleepTime);

	RTC_LOG(LS_INFO) << "kimi_thread_check_trace RTCTaskQueuePacedSender tid " << (unsigned int)gettid();

	burst_interval_ = burst_interval;

	if (burst_interval_.has_value()) {
		pacing_controller_.SetSendBurstInterval(burst_interval_.value());
	}
}

RTCTaskQueuePacedSender::~RTCTaskQueuePacedSender() {
	RTC_DCHECK_RUN_ON(task_queue_);
	is_shutdown_ = true;
}

void RTCTaskQueuePacedSender::EnsureStarted() {
	RTC_DCHECK_RUN_ON(task_queue_);
	is_started_ = true;
	MaybeProcessPackets(Timestamp::MinusInfinity());
}

void RTCTaskQueuePacedSender::EnqueuePackets(
	std::vector<std::unique_ptr<RtpPacketToSend>> packets) {

		task_queue_->PostTask(
		SafeTask(safety_.flag(), [this, packets = std::move(packets)]() mutable {
		RTC_DCHECK_RUN_ON(task_queue_);
		for (auto& packet : packets) {
			size_t packet_size = packet->payload_size() + packet->padding_size();
			if (include_overhead_) {
				packet_size += packet->headers_size();
			}
			packet_size_.Apply(1, packet_size);
			RTC_DCHECK_GE(packet->capture_time_ms(), 0ll);
			pacing_controller_.EnqueuePacket(std::move(packet));
		}
		MaybeProcessPackets(Timestamp::MinusInfinity());
	}));
}

void RTCTaskQueuePacedSender::EnqueuePacket(std::unique_ptr<RtpPacketToSend> packet) {
	int64_t now = rtc::TimeMillis();
#ifdef __ENABLE_DEBUGING_PACKET_PATH_TRACE__
	PACKET_TRACE_INFO dinfo;
	dinfo._trace_info = "task_queue_paced_sender_enqueue_packet";
	dinfo._time_us = rtc::TimeMicros();
	packet->_debug_packet_trace_vector.push_back(dinfo);
#endif

#ifdef __PERFORMANCE_PROFILE_ENABLED__  
	{
        RTCScopedSectionTimer enque_timer(_prof_post_enqueue);
	}
#endif  

	task_queue_->PostTask(
		SafeTask(safety_.flag(), [this, packet = std::move(packet)]() mutable {
		RTC_DCHECK_RUN_ON(task_queue_);

		size_t packet_size = packet->payload_size() + packet->padding_size();
		if (include_overhead_) {
			packet_size += packet->headers_size();
		}
		packet_size_.Apply(1, packet_size);
		RTC_DCHECK_GE(packet->capture_time_ms(), 0ll);
		pacing_controller_.EnqueuePacket(std::move(packet));
		MaybeProcessPackets(Timestamp::MinusInfinity());
	}));

#ifdef __PERFORMANCE_PROFILE_ENABLED__    
    if (_prof_post_enqueue.ReportDue()) {
		 RTC_LOG(LS_ERROR)<<"profile_trace "<< _prof_post_enqueue.TakeReport().c_str();
    }
#endif 	
}

void RTCTaskQueuePacedSender::RemovePacketsForSsrc(uint32_t ssrc) {
	task_queue_->PostTask(SafeTask(safety_.flag(), [this, ssrc] {
		RTC_DCHECK_RUN_ON(task_queue_);
		pacing_controller_.RemovePacketsForSsrc(ssrc);

		MaybeProcessPackets(Timestamp::MinusInfinity());
	}));
}

void RTCTaskQueuePacedSender::CreateProbeClusters(
	std::vector<ProbeClusterConfig> probe_cluster_configs) {
	RTC_DCHECK_RUN_ON(task_queue_);
	pacing_controller_.CreateProbeClusters(probe_cluster_configs);
	MaybeScheduleProcessPackets();
}

void RTCTaskQueuePacedSender::Pause() {
	RTC_DCHECK_RUN_ON(task_queue_);
	pacing_controller_.Pause();
}

void RTCTaskQueuePacedSender::Resume() {
	RTC_DCHECK_RUN_ON(task_queue_);
	pacing_controller_.Resume();
	MaybeProcessPackets(Timestamp::MinusInfinity());
}

void RTCTaskQueuePacedSender::SetCongested(bool congested) {
	RTC_DCHECK_RUN_ON(task_queue_);
	pacing_controller_.SetCongested(congested);
	MaybeScheduleProcessPackets();
}

void RTCTaskQueuePacedSender::SetPacingRates(DataRate pacing_rate,
	DataRate padding_rate) {
	RTC_DCHECK_RUN_ON(task_queue_);
	pacing_controller_.SetPacingRates(pacing_rate, padding_rate);
	MaybeScheduleProcessPackets();
}

void RTCTaskQueuePacedSender::SetAccountForAudioPackets(bool account_for_audio) {
	RTC_DCHECK_RUN_ON(task_queue_);
	pacing_controller_.SetAccountForAudioPackets(account_for_audio);
	MaybeProcessPackets(Timestamp::MinusInfinity());
}

void RTCTaskQueuePacedSender::SetIncludeOverhead() {
	RTC_DCHECK_RUN_ON(task_queue_);
	include_overhead_ = true;
	pacing_controller_.SetIncludeOverhead();
	MaybeProcessPackets(Timestamp::MinusInfinity());
}

void RTCTaskQueuePacedSender::SetTransportOverhead(DataSize overhead_per_packet) {
	RTC_DCHECK_RUN_ON(task_queue_);
	pacing_controller_.SetTransportOverhead(overhead_per_packet);
	MaybeProcessPackets(Timestamp::MinusInfinity());
}

TimeDelta RTCTaskQueuePacedSender::OldestPacketWaitTime() const {
	Timestamp oldest_packet = GetStats().oldest_packet_enqueue_time;
	if (oldest_packet.IsInfinite()) {
		return TimeDelta::Zero();
	}

	// (webrtc:9716): The clock is not always monotonic.
	Timestamp current = clock_->CurrentTime();
	if (current < oldest_packet) {
		return TimeDelta::Zero();
	}

	return current - oldest_packet;
}

DataSize RTCTaskQueuePacedSender::QueueSizeData() const {
	return GetStats().queue_size;
}

absl::optional<Timestamp> RTCTaskQueuePacedSender::FirstSentPacketTime() const {
	return GetStats().first_sent_packet_time;
}

TimeDelta RTCTaskQueuePacedSender::ExpectedQueueTime() const {
	return GetStats().expected_queue_time;
}

void RTCTaskQueuePacedSender::SetQueueTimeLimit(TimeDelta limit) {
	RTC_DCHECK_RUN_ON(task_queue_);
	pacing_controller_.SetQueueTimeLimit(limit);
	MaybeProcessPackets(Timestamp::MinusInfinity());
}

void RTCTaskQueuePacedSender::MaybeScheduleProcessPackets() {
	if (!processing_packets_)
		MaybeProcessPackets(Timestamp::MinusInfinity());
}

void RTCTaskQueuePacedSender::MaybeProcessPackets(
	Timestamp scheduled_process_time) {
	RTC_DCHECK_RUN_ON(task_queue_);

	if (is_shutdown_ || !is_started_) {
		return;
	}

	RTC_DCHECK(!processing_packets_);
	processing_packets_ = true;
	absl::Cleanup cleanup = [this] {
		RTC_DCHECK_RUN_ON(task_queue_);
		processing_packets_ = false;
	};

	Timestamp next_send_time = pacing_controller_.NextSendTime();
	RTC_DCHECK(next_send_time.IsFinite());
	const Timestamp now = clock_->CurrentTime();

	TimeDelta early_execute_margin =
		pacing_controller_.IsProbing()
		? RTCPacingController::kMaxEarlyProbeProcessing
		: TimeDelta::Zero();

	while (next_send_time <= now + early_execute_margin) {
		pacing_controller_.ProcessPackets();
		next_send_time = pacing_controller_.NextSendTime();
		RTC_DCHECK(next_send_time.IsFinite());
		early_execute_margin = pacing_controller_.IsProbing()
			? RTCPacingController::kMaxEarlyProbeProcessing
			: TimeDelta::Zero();
	}

	UpdateStats();

	if (scheduled_process_time.IsFinite()) {
		if (scheduled_process_time != next_process_time_) {
			return;
		}
		next_process_time_ = Timestamp::MinusInfinity();
	}

	// probing
	TimeDelta hold_back_window = TimeDelta::Zero();
	if (!pacing_controller_.IsProbing()) {
		hold_back_window = max_hold_back_window_;
		DataRate pacing_rate = pacing_controller_.pacing_rate();
		if (max_hold_back_window_in_packets_ != kNoPacketHoldback &&
			!pacing_rate.IsZero() &&
			packet_size_.filtered() != rtc::ExpFilter::kValueUndefined) {
			TimeDelta avg_packet_send_time =
				DataSize::Bytes(packet_size_.filtered()) / pacing_rate;
			hold_back_window =
				std::min(hold_back_window,
				avg_packet_send_time * max_hold_back_window_in_packets_);
		}
	}

	// Calculate next process time.
	TimeDelta time_to_next_process =
		std::max(hold_back_window, next_send_time - now - early_execute_margin);

	next_send_time = now + time_to_next_process;

	if (next_process_time_.IsMinusInfinity() ||
		next_process_time_ > next_send_time) {
		task_queue_->PostDelayedHighPrecisionTask(
			SafeTask(
			safety_.flag(),
			[this, next_send_time]() { MaybeProcessPackets(next_send_time); }),
			time_to_next_process.RoundUpTo(TimeDelta::Millis(1)));
		next_process_time_ = next_send_time;
	}
}

void RTCTaskQueuePacedSender::OnStatsUpdated(const Stats& stats) {
	RTC_DCHECK_RUN_ON(task_queue_);
	current_stats_ = stats;
}

void RTCTaskQueuePacedSender::UpdateStats() {
	Stats new_stats;
	new_stats.expected_queue_time = pacing_controller_.ExpectedQueueTime();
	new_stats.first_sent_packet_time = pacing_controller_.FirstSentPacketTime();
	new_stats.oldest_packet_enqueue_time =
		pacing_controller_.OldestPacketEnqueueTime();
	new_stats.queue_size = pacing_controller_.QueueSizeData();
	OnStatsUpdated(new_stats);
}

RTCTaskQueuePacedSender::Stats RTCTaskQueuePacedSender::GetStats() const {
	RTC_DCHECK_RUN_ON(task_queue_);
	return current_stats_;
}

}