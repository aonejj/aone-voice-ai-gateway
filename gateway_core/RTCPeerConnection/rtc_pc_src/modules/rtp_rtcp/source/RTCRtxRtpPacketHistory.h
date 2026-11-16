//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_MODULES_RTP_RTCP_SOURCE_RTX_RTP_PACKET_H__
#define __RTC_MODULES_RTP_RTCP_SOURCE_RTX_RTP_PACKET_H__

#include <map>
#include <memory>
#include <list>

#include "api/function_view.h"
#include "modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "rtc_base/synchronization/mutex.h"
#include "rtc_base/thread_annotations.h"
#include "rtc_base/constructor_magic.h"

namespace webrtc {

class RTCRtxRtpPacketHistory {
public:
	enum class RtxStorageMode {
		kRtxDisabled,
		kRtxStoreAndCull
	};

	struct RtxPacketState {
		RtxPacketState();
		RtxPacketState(const RtxPacketState&);
		~RtxPacketState();

		uint16_t rtp_sequence_number = 0;
		uint32_t ssrc = 0;
		size_t packet_size = 0;
		// Number of times RE-transmitted, ie not including the first transmission.
		size_t times_retransmitted = 0;
		bool pending_transmission = false;
	};

	// Maximum number of packets we ever allow in the history.
	static constexpr size_t kMaxCapacity = 9600;
	// Maximum number of entries in prioritized queue of padding packets.
	static constexpr size_t kMaxPaddingtHistory = 63;
	// Don't remove packets within max(1000ms, 3x RTT).
	static constexpr int64_t kMinPacketDurationMs = 1000;
	static constexpr int kMinPacketDurationRtt = 3;
	// With kStoreAndCull, always remove packets after 3x max(1000ms, 3x rtt).
	static constexpr int kPacketCullingDelayFactor = 3;

	RTCRtxRtpPacketHistory(Clock *clock);
	RTCRtxRtpPacketHistory() = delete;

	~RTCRtxRtpPacketHistory();

public:
	void SetStoreRtxPacketsStatus(RtxStorageMode mode, size_t number_to_store);
	RtxStorageMode GetRtxStorageMode() const;

	void SetRtt(int64_t rtt_ms);
	void PutRtxRtpPacket(std::unique_ptr<RtpPacketToSend> rtx_packet,
		absl::optional<int64_t> send_time_ms);

	std::unique_ptr<RtpPacketToSend> GetRtxPacketAndMarkAsPending(
		uint16_t sequence_number,
		rtc::FunctionView<std::unique_ptr<RtpPacketToSend>(const RtpPacketToSend&)> 
		encapsulate);

	void MarkRtxPacketAsSent(uint16_t sequence_number);

	absl::optional<RtxPacketState> GetRtxPacketState(uint16_t sequence_number);

	void CullAcknowledgedRtxPackets(rtc::ArrayView<const uint16_t> sequence_numbers);

	void Clear();

private:
	class RtxStoredPacket {
	public:
		RtxStoredPacket(std::unique_ptr<RtpPacketToSend> packet,
			absl::optional<int64_t> send_time_ms,
			uint16_t seq_no);
		RtxStoredPacket(RtxStoredPacket&&);
		RtxStoredPacket& operator=(RtxStoredPacket&&);
		~RtxStoredPacket();

		size_t times_retransmitted() const { return times_retransmitted_; }
		void IncrementTimesRetransmitted();

		uint16_t rtp_sequence_number_;
		absl::optional<int64_t> send_time_ms_;
		std::unique_ptr<RtpPacketToSend> rtx_packet_;
		bool pending_transmission_;

	private:
		size_t times_retransmitted_;
	};

private:
	bool VerifyRtt(const RtxStoredPacket& packet, int64_t now_ms) const
		RTC_EXCLUSIVE_LOCKS_REQUIRED(lock_);
	
	void Reset() RTC_EXCLUSIVE_LOCKS_REQUIRED(lock_);
	void CullOldPackets(int64_t now_ms)  RTC_EXCLUSIVE_LOCKS_REQUIRED(lock_);
	std::unique_ptr<RtpPacketToSend> RemovePacket(uint16_t sequence_number)
		RTC_EXCLUSIVE_LOCKS_REQUIRED(lock_);
	void PopFront() RTC_EXCLUSIVE_LOCKS_REQUIRED(lock_);
	RtxStoredPacket* FindRtxStoredPacket(uint16_t sequence_number)
		RTC_EXCLUSIVE_LOCKS_REQUIRED(lock_);
	std::list<RtxStoredPacket>::iterator IsFindRtxStoredPacket(uint16_t sequence_number)
		RTC_EXCLUSIVE_LOCKS_REQUIRED(lock_);

	static RtxPacketState StoredRtxPacketToPacketState(
		const RtxStoredPacket& stored_packet);

private:
	Clock* const clock_;
	mutable Mutex lock_;
	size_t number_to_store_ RTC_GUARDED_BY(lock_);
	RtxStorageMode mode_ RTC_GUARDED_BY(lock_);
	int64_t rtt_ms_ RTC_GUARDED_BY(lock_);

	std::list<RtxStoredPacket> packet_history_list_ RTC_GUARDED_BY(lock_);
	std::map<uint16_t, std::list<RtxStoredPacket>::iterator > packet_indexer_ RTC_GUARDED_BY(lock_);

private:
	RTC_DISALLOW_COPY_AND_ASSIGN(RTCRtxRtpPacketHistory);
};

}	// namespace webrtc

#endif // __RTC_MODULES_RTP_RTCP_SOURCE_RTX_RTP_PACKET_H__