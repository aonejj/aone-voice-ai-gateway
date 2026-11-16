//////////////////////////////////////////////////////////////////////////
//
// author : kimi   deprecated
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_RTCP_HANDLER_H__
#define __RTC_RTCP_HANDLER_H__

#include "modules/rtp_rtcp/source/rtcp_sender.h"
#include "modules/rtp_rtcp/source/rtcp_receiver.h"
#include "modules/rtp_rtcp/source/rtp_packet_history.h"
#include "modules/rtp_rtcp/source/rtp_rtcp_interface.h"
#include "rtc_base/ref_count.h"
#include "system_wrappers/include/clock.h"

#include "../../../api/call/RTCTransportSequenceNumberGenerator.h"
#include "../../../call/RTCVideoSendStream.h"
#include "../../../call/RTCAudioSendStream.h"


namespace webrtc {

class RTCRtcpHandler : public rtc::RefCountInterface {
public:
	static rtc::scoped_refptr<RTCRtcpHandler> Create(
		Clock* clock,
		RtpRtcpInterface::Configuration &rtcp_config,
		RtcpRttStats* rtt_stats, RTCPReceiver::ModuleRtpRtcp* owner,
		bool isVideo = true);

protected:
	RTCRtcpHandler(
		Clock* clock,
		RtpRtcpInterface::Configuration &rtcp_config,
		RtcpRttStats* rtt_stats, RTCPReceiver::ModuleRtpRtcp* owner,
		bool isVideo);

public:
	~RTCRtcpHandler();

public:
	void SetVideoSendStreamConfig(const RTCVideoSendStream::Config* const config);
	void SetAudioSendStreamConfig(const RTCAudioSendStream::Config* const config);
	void SendRTCPReport(const int64_t nowMs);
	void DeliverRtcp(const uint8_t* packet, size_t length);
	void SetSendingStatus(bool enabled);
	void SetLastRtpTime(uint32_t rtp_timestamp,
		int64_t capture_time_ms,
		int8_t payload_type = -1);
	absl::optional<TimeDelta> PeriodicUpdate();
	void CheckTMMBR();
	void SetRtt(int64_t rtt_ms);

	int32_t SendRTCP(RTCPPacketType packet_type);

	int32_t RemoteRTCPStat(
		std::vector<RTCPReportBlock>* receive_blocks) const;

	int32_t RTT(uint32_t remote_ssrc,
		int64_t* rtt,
		int64_t* avg_rtt,
		int64_t* min_rtt,
		int64_t* max_rtt) const;

	void PutRtpPacketInHistory(std::unique_ptr<RtpPacketToSend> packet);
	bool StorePackets() const;
	int32_t ReSendPacket(uint16_t packet_id);

	void OnReceivedNack(const std::vector<uint16_t>& nack_sequence_numbers);
	void OnPacketFeedbackVector(
		std::vector<StreamFeedbackObserver::StreamPacketInfo>& packet_feedback_vector);


	bool TMMBR() const;
	void SetTmmbn(std::vector<rtcp::TmmbItem> bounding_set);

	void MaybeSendRtcp() RTC_RUN_ON(worker_queue_);
	void MaybeSendRtcpAtOrAfterTimestamp(Timestamp execution_time)
		RTC_RUN_ON(worker_queue_);

	void ScheduleRtcpSendEvaluation(TimeDelta duration);

	// Schedules a call to MaybeSendRtcpAtOrAfterTimestamp delayed by `duration`.
	void ScheduleMaybeSendRtcpAtOrAfterTimestamp(Timestamp execution_time,
		TimeDelta duration);

	static std::unique_ptr<RtpPacketToSend> RtpPacketReceivedToSend(
		const RtpPacketReceived *received_packet, RtpHeaderExtensionMap& header_extension_map, bool isVideo = true);

private:
	void SetRtxPayloadType(int payload_type,
		int associated_payload_type);
	RTCPSender::FeedbackState _get_feedback_state();
	bool _last_received_NTP(uint32_t* NTPsecs, uint32_t* NTPfrac, uint32_t* remote_sr) const;
	int32_t _resend_packet_in_history(uint16_t packet_id, bool &transmit_pending);
	std::unique_ptr<RtpPacketToSend> BuildRtxPacket(const RtpPacketToSend& packet);
	void send_rtx_packet(std::unique_ptr<RtpPacketToSend> packet,
		 bool is_rtx_fw = false);

	void set_rtt_ms(int64_t rtt_ms);
	int64_t rtt_ms() const;

private:
	Clock* const clock_;
	Transport* send_transport_;
	RTCTransportSequenceNumberGenerator* transport_seq_generator_;
	RtpHeaderExtensionMap header_extension_map_;

	RTCPSender rtcp_sender_;
	RTCPReceiver rtcp_receiver_;
	RtcpRttStats* rtt_stats_;

	// only video variables
	RtpPacketHistory packet_history_;

	size_t max_packet_size_;
	uint16_t sequence_number_rtx_;
	Random random_;
	absl::optional<uint32_t> rtx_ssrc_;
	std::map<int8_t, int8_t> rtx_payload_type_map_;	

	mutable Mutex send_mutex_;
	bool sending_media_ RTC_GUARDED_BY(send_mutex_);

	bool is_video_;

	int64_t last_rtt_process_time_;

	mutable Mutex mutex_rtt_;
	int64_t rtt_ms_ RTC_GUARDED_BY(mutex_rtt_);
};

}


#endif	// __RTC_RTCP_HANDLER_H__