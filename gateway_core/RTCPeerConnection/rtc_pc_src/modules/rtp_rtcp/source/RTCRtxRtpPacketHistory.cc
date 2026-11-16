//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////


#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

#include "modules/include/module_common_types_public.h"
#include "modules/rtp_rtcp/source/rtp_packet_to_send.h"
#include "modules/rtp_rtcp/source/rtp_packet_received.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "system_wrappers/include/clock.h"

#include "RTCRtxRtpPacketHistory.h"


//#define __RTX_HISTORY_PERFORMANCE_CHECK__

namespace webrtc {

constexpr size_t RTCRtxRtpPacketHistory::kMaxCapacity;
constexpr size_t RTCRtxRtpPacketHistory::kMaxPaddingtHistory;
constexpr int64_t RTCRtxRtpPacketHistory::kMinPacketDurationMs;
constexpr int RTCRtxRtpPacketHistory::kMinPacketDurationRtt;
constexpr int RTCRtxRtpPacketHistory::kPacketCullingDelayFactor;

RTCRtxRtpPacketHistory::RtxPacketState::RtxPacketState() = default;
RTCRtxRtpPacketHistory::RtxPacketState::RtxPacketState(const RtxPacketState&) = default;
RTCRtxRtpPacketHistory::RtxPacketState::~RtxPacketState() = default;

void RTCRtxRtpPacketHistory::RtxStoredPacket::IncrementTimesRetransmitted() {
	++times_retransmitted_;
}

RTCRtxRtpPacketHistory::RtxStoredPacket::RtxStoredPacket(
	std::unique_ptr<RtpPacketToSend> packet,
	absl::optional<int64_t> send_time_ms,
	uint16_t seq_no)
	: send_time_ms_(send_time_ms),
	  rtx_packet_(std::move(packet)),
	  pending_transmission_(!send_time_ms.has_value()),
	  rtp_sequence_number_(seq_no),
	  times_retransmitted_(0) {
}

RTCRtxRtpPacketHistory::RtxStoredPacket::RtxStoredPacket(RtxStoredPacket&&) = default;
RTCRtxRtpPacketHistory::RtxStoredPacket& RTCRtxRtpPacketHistory::RtxStoredPacket::operator=(
	RTCRtxRtpPacketHistory::RtxStoredPacket&&) = default;
RTCRtxRtpPacketHistory::RtxStoredPacket::~RtxStoredPacket() = default;


RTCRtxRtpPacketHistory::RTCRtxRtpPacketHistory(Clock* clock)
	: clock_(clock),
	number_to_store_(0),
	mode_(RtxStorageMode::kRtxDisabled),
	rtt_ms_(-1) {
}

RTCRtxRtpPacketHistory::~RTCRtxRtpPacketHistory() {}

void RTCRtxRtpPacketHistory::SetStoreRtxPacketsStatus(RtxStorageMode mode,
	size_t number_to_store) {
	RTC_DCHECK_LE(number_to_store, kMaxCapacity);
	MutexLock lock(&lock_);
	if (mode != RtxStorageMode::kRtxDisabled && mode_ != RtxStorageMode::kRtxDisabled) {
		RTC_LOG(LS_WARNING) << "Purging packet history in order to re-set status.";
	}

	Reset();

	mode_ = mode;
	number_to_store_ = std::min(kMaxCapacity, number_to_store);
}

RTCRtxRtpPacketHistory::RtxStorageMode RTCRtxRtpPacketHistory::GetRtxStorageMode() const {
	MutexLock lock(&lock_);
	return mode_;
}

void RTCRtxRtpPacketHistory::SetRtt(int64_t rtt_ms) {
	MutexLock lock(&lock_);
	RTC_DCHECK_GE(rtt_ms, 0);
	rtt_ms_ = rtt_ms;
	// If storage is not disabled,  packets will be removed after a timeout
	// that depends on the RTT. Changing the RTT may thus cause some packets
	// become "old" and subject to removal.
	if (mode_ != RtxStorageMode::kRtxDisabled) {
		CullOldPackets(clock_->TimeInMilliseconds());
	}
}

void RTCRtxRtpPacketHistory::PutRtxRtpPacket(std::unique_ptr<RtpPacketToSend> rtx_packet,
	absl::optional<int64_t> send_time_ms) {
	RTC_DCHECK(rtx_packet);
	MutexLock lock(&lock_);
	int64_t now_ms = clock_->TimeInMilliseconds();

#ifdef __RTX_HISTORY_PERFORMANCE_CHECK__	
	int64_t proc_now_us = clock_->TimeInMicroseconds();
#endif

	if (mode_ == RtxStorageMode::kRtxDisabled) {
		return;
	}

	CullOldPackets(now_ms);

	rtc::ArrayView<const uint8_t> payload = rtx_packet->payload();
	uint16_t seq_no = (payload[0] << 8) + payload[1];

	auto it = IsFindRtxStoredPacket(seq_no);
	if (it != packet_history_list_.end()) {
		// duplicate
		return;
	}

	packet_history_list_.push_back(RtxStoredPacket(std::move(rtx_packet), send_time_ms, seq_no));
	packet_indexer_[seq_no] = std::prev(packet_history_list_.end());

#ifdef __RTX_HISTORY_PERFORMANCE_CHECK__
	RTC_LOG(LS_INFO) << "["<<this<<"] RTCRtxRtpPacketHistory::PutRtxRtpPacket proc us \t" << clock_->TimeInMicroseconds() - proc_now_us;
#endif

}

std::unique_ptr<RtpPacketToSend> RTCRtxRtpPacketHistory::GetRtxPacketAndMarkAsPending(
	uint16_t sequence_number,
	rtc::FunctionView<std::unique_ptr<RtpPacketToSend>(const RtpPacketToSend&)>
	encapsulate) {
	MutexLock lock(&lock_);
	if (mode_ == RtxStorageMode::kRtxDisabled) {
		return nullptr;
	}

#ifdef __RTX_HISTORY_PERFORMANCE_CHECK__
	int64_t now_us = clock_->TimeInMicroseconds();
#endif

	RtxStoredPacket* packet = FindRtxStoredPacket(sequence_number);
	
	if (packet == nullptr) {
		return nullptr;
	}

	if (packet->pending_transmission_) {
		// Packet already in pacer queue, ignore this request.
		return nullptr;
	}

	// Copy and/or encapsulate packet.
	std::unique_ptr<RtpPacketToSend> encapsulated_packet =
		encapsulate(*packet->rtx_packet_);
	if (encapsulated_packet) {
		packet->pending_transmission_ = true;
	}

#ifdef __RTX_HISTORY_PERFORMANCE_CHECK__
	RTC_LOG(LS_INFO) << "["<<this<<"] RTCRtxRtpPacketHistory::GetRtxPacketAndMarkAsPending proc us \t" << clock_->TimeInMicroseconds() - now_us;
#endif

	return encapsulated_packet;
}

void RTCRtxRtpPacketHistory::MarkRtxPacketAsSent(uint16_t sequence_number) {
	MutexLock lock(&lock_);
	if (mode_ == RtxStorageMode::kRtxDisabled) {
		return;
	}

#ifdef __RTX_HISTORY_PERFORMANCE_CHECK__	
	int64_t now_us = clock_->TimeInMicroseconds();
#endif

	RtxStoredPacket* packet = FindRtxStoredPacket(sequence_number);

	if (packet == nullptr) {
		return;
	}

	RTC_DCHECK(packet->send_time_ms_);

	// Update send-time, mark as no longer in pacer queue, and increment
	// transmission count.
	packet->send_time_ms_ = clock_->TimeInMilliseconds();
	packet->pending_transmission_ = false;
	packet->IncrementTimesRetransmitted();

#ifdef __RTX_HISTORY_PERFORMANCE_CHECK__	
	RTC_LOG(LS_INFO) << "["<<this<<"] RTCRtxRtpPacketHistory::MarkRtxPacketAsSent proc us \t" << clock_->TimeInMicroseconds() - now_us;
#endif
}

void RTCRtxRtpPacketHistory::CullAcknowledgedRtxPackets(
	rtc::ArrayView<const uint16_t> sequence_numbers) {
	MutexLock lock(&lock_);

#ifdef __RTX_HISTORY_PERFORMANCE_CHECK__	
	int64_t now_us = clock_->TimeInMicroseconds();
#endif

	for (uint16_t sequence_number : sequence_numbers) {
		RemovePacket(sequence_number);
	}

#ifdef __RTX_HISTORY_PERFORMANCE_CHECK__	
	RTC_LOG(LS_INFO) << "["<<this<<"] RTCRtxRtpPacketHistory::CullAcknowledgedRtxPackets proc us \t" << clock_->TimeInMicroseconds() - now_us;
#endif
}

bool RTCRtxRtpPacketHistory::VerifyRtt(const RTCRtxRtpPacketHistory::RtxStoredPacket& packet,
	int64_t now_ms) const {

	if (packet.send_time_ms_) {
		if (packet.times_retransmitted() > 0 &&
			now_ms < *packet.send_time_ms_ + rtt_ms_) {
			return false;
		}
	}

	return true;
}

absl::optional<RTCRtxRtpPacketHistory::RtxPacketState> RTCRtxRtpPacketHistory::GetRtxPacketState(
	uint16_t sequence_number) {
	MutexLock lock(&lock_);
	
	if (mode_ == RtxStorageMode::kRtxDisabled) {
		return absl::nullopt;
	}


#ifdef __RTX_HISTORY_PERFORMANCE_CHECK__	
	int64_t now_us = clock_->TimeInMicroseconds();
#endif

	auto it = IsFindRtxStoredPacket(sequence_number);
	if (it == packet_history_list_.end()) {
		return absl::nullopt;
	}

	const RtxStoredPacket& packet = *it;
	if (packet.rtx_packet_ == nullptr) {
		return absl::nullopt;
	}

#ifdef __RTX_HISTORY_PERFORMANCE_CHECK__	
	RTC_LOG(LS_INFO) << "["<<this<<"] RTCRtxRtpPacketHistory::GetRtxPacketState proc us \t" << clock_->TimeInMicroseconds() - now_us;
#endif 

	return StoredRtxPacketToPacketState(packet);
}

void RTCRtxRtpPacketHistory::Clear() {
	MutexLock lock(&lock_);
	Reset();
}

//////////////////////////////////////////////////////////////////////////
// plan B methods
void RTCRtxRtpPacketHistory::Reset() {

	while (packet_history_list_.begin() != packet_history_list_.end()) {
		std::unique_ptr<RtpPacketToSend> rtx_rtp_packet =
			std::move(packet_history_list_.front().rtx_packet_);

		packet_history_list_.pop_front();
	}

	packet_indexer_.clear();
}

void RTCRtxRtpPacketHistory::CullOldPackets(int64_t now_ms) {
	int64_t packet_duration_ms =
		std::max(kMinPacketDurationRtt * rtt_ms_, kMinPacketDurationMs);

	while (!packet_history_list_.empty()) {
		if (packet_history_list_.size() >= number_to_store_) {
			PopFront();
			continue;
		}

		const RtxStoredPacket& stored_packet = packet_history_list_.front();
		if (stored_packet.pending_transmission_) {
			// Don't remove packets in the pacer queue, pending tranmission.
			return;
		}

		if (*stored_packet.send_time_ms_ + packet_duration_ms > now_ms) {
			// Don't cull packets too early to avoid failed retransmission requests.
			return;
		}

		if ( *stored_packet.send_time_ms_ + (packet_duration_ms * kPacketCullingDelayFactor) <= now_ms) {
			// Too many packets in history, or this packet has timed out. Remove it
			// and continue.
			PopFront();
		}
		else {
			// No more packets can be removed right now.
			return;
		}
	}
}

std::unique_ptr<RtpPacketToSend> RTCRtxRtpPacketHistory::RemovePacket(uint16_t sequence_number) {
	auto it = packet_indexer_.find(sequence_number);
	if (it == packet_indexer_.end()) {
		return nullptr;
	}

	RtxStoredPacket& stored_packet = *(it->second);
	packet_history_list_.erase(it->second);
	packet_indexer_.erase(it);

	std::unique_ptr<RtpPacketToSend> rtx_rtp_packet = std::move(stored_packet.rtx_packet_);

	return rtx_rtp_packet;
}

void RTCRtxRtpPacketHistory::PopFront() {
	RtxStoredPacket& stored_packet = packet_history_list_.front();
	auto it = packet_indexer_.find(stored_packet.rtp_sequence_number_);
	packet_indexer_.erase(it);
	std::unique_ptr<RtpPacketToSend> rtx_rtp_packet = std::move(stored_packet.rtx_packet_);
	packet_history_list_.pop_front();
}

RTCRtxRtpPacketHistory::RtxStoredPacket* RTCRtxRtpPacketHistory::FindRtxStoredPacket(uint16_t sequence_number) {
	std::list<RtxStoredPacket>::iterator it = IsFindRtxStoredPacket(sequence_number);
	if (it == packet_history_list_.end()) {
		return nullptr;
	}

	return &(*it);
}

std::list<RTCRtxRtpPacketHistory::RtxStoredPacket>::iterator
RTCRtxRtpPacketHistory::IsFindRtxStoredPacket(uint16_t sequence_number) {
	auto it = packet_indexer_.find(sequence_number);
	if (it == packet_indexer_.end()) {
		return packet_history_list_.end();
	}

	return it->second;
}
//////////////////////////////////////////////////////////////////////////

RTCRtxRtpPacketHistory::RtxPacketState RTCRtxRtpPacketHistory::StoredRtxPacketToPacketState(
	const RTCRtxRtpPacketHistory::RtxStoredPacket& stored_packet) {
	RTCRtxRtpPacketHistory::RtxPacketState state;
	state.rtp_sequence_number = stored_packet.rtp_sequence_number_;
	state.ssrc = stored_packet.rtx_packet_->Ssrc();
	state.packet_size = stored_packet.rtx_packet_->size();
	state.times_retransmitted = stored_packet.times_retransmitted();
	state.pending_transmission = stored_packet.pending_transmission_;
	return state;
}

}