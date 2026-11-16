//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#include "rtc_base/logging.h"
#include "modules/rtp_rtcp/source/byte_io.h"

#include "modules/rtp_rtcp/source/rtp_utility.h"

#include "RTCRtcpHandler.h"

namespace webrtc {

static const int kMinSendSidePacketHistorySize = 1000; // 600;
constexpr uint16_t kMaxInitRtpSeqNumber = 32767;  // 2^15 -1.
constexpr TimeDelta kRttUpdateInterval = TimeDelta::Millis(1000);
const int64_t kRtpRtcpRttProcessTimeMs = 1000;
static const int kMinSendSideRtxPacketHistorySize = 100;

#define INIT_SIMULCAST_PROXY_TIMESTAMP			270000	

RTCPSender::Configuration AddRtcpSendEvaluationCallback(
	RTCPSender::Configuration config,
	std::function<void(TimeDelta)> send_evaluation_callback) {
	config.schedule_next_rtcp_send_evaluation_function =
		std::move(send_evaluation_callback);
	return config;
}

rtc::scoped_refptr<RTCRtcpHandler> RTCRtcpHandler::Create(
	Clock* clock, RtpRtcpInterface::Configuration &rtcp_config,
	RtcpRttStats* rtt_stats, RTCPReceiver::ModuleRtpRtcp* owner,
	bool isVideo /* = true */) {

	rtc::scoped_refptr<RTCRtcpHandler> handler(new rtc::RefCountedObject<RTCRtcpHandler>(clock, rtcp_config, rtt_stats, owner, isVideo));

	return handler;
}

RTCRtcpHandler::RTCRtcpHandler(
	Clock* clock, RtpRtcpInterface::Configuration &rtcp_config,
	RtcpRttStats* rtt_stats, RTCPReceiver::ModuleRtpRtcp* owner,
	bool isVideo)
	: clock_(clock),
	send_transport_(nullptr),
	transport_seq_generator_(nullptr),
	rtcp_sender_(AddRtcpSendEvaluationCallback(
		RTCPSender::Configuration::FromRtpRtcpConfiguration(rtprtcp_config),
			[this](TimeDelta duration) {
			ScheduleRtcpSendEvaluation(duration);
		})),
	rtcp_receiver_(rtcp_config, owner),
	rtt_stats_(rtt_stats),
	packet_history_(clock, rtcp_config.enable_rtx_padding_prioritization),
	max_packet_size_(IP_PACKET_SIZE - 28),
	random_(clock_->TimeInMicroseconds()),
	sending_media_(false),
	is_video_(isVideo),
	last_rtt_process_time_(clock_->TimeInMilliseconds()),
	rtt_ms_(0) {
	if (is_video_) {
		RTC_LOG(LS_VERBOSE) << "[" << this << "] RTCRtcpHandler::RTCRtcpHandler video rtcp_config.receiver_only " << rtcp_config.receiver_only;
	}

	if (!rtcp_config.receiver_only) {
		rtcp_sender_.SetTimestampOffset(INIT_SIMULCAST_PROXY_TIMESTAMP);
	}

	if (is_video_) {
		packet_history_.SetStorePacketsStatus(
			RtpPacketHistory::StorageMode::kStoreAndCull,
			kMinSendSidePacketHistorySize);
	}
}

RTCRtcpHandler::~RTCRtcpHandler() {
	RTC_LOG(LS_VERBOSE) << "[" << this << "] RTCRtcpHandler::~RTCRtcpHandler " << (is_video_ ? " video " : " audio ");
}

void RTCRtcpHandler::SetVideoSendStreamConfig(const RTCVideoSendStream::Config* const config) {
	rtcp_sender_.SetRTCPStatus(RtcpMode::kCompound);
	rtcp_sender_.SetCNAME(config->rtp.c_name.c_str());
	rtcp_sender_.SetMaxRtpPacketSize(config->rtp.max_packet_size);
	rtcp_sender_.SetRtpClockRate(config->rtp.payload_type, kVideoPayloadTypeFrequency);
	SetRtxPayloadType(config->rtp.rtx.payload_type, config->rtp.payload_type);
	sequence_number_rtx_ = random_.Rand(1, kMaxInitRtpSeqNumber);

	header_extension_map_ = RtpHeaderExtensionMap(config->rtp.extensions);
	send_transport_ = config->send_transport;
	transport_seq_generator_ = config->transport_sequence_number_generator;
	rtx_ssrc_ = config->rtp.rtx.ssrcs[0];
}

void RTCRtcpHandler::SetAudioSendStreamConfig(const RTCAudioSendStream::Config* const config) {
	rtcp_sender_.SetRTCPStatus(RtcpMode::kCompound);
	rtcp_sender_.SetCNAME(config->rtp.c_name.c_str());
	rtcp_sender_.SetRtpClockRate(config->send_codec_spec->payload_type, config->send_codec_spec->format.clockrate_hz);

	header_extension_map_ = RtpHeaderExtensionMap(config->rtp.extensions);
	send_transport_ = config->send_transport;
}

void RTCRtcpHandler::SendRTCPReport(const int64_t nowMs) {
	MutexLock lock(&send_mutex_);
	if (!sending_media_) {
		return;
	}

	if (rtcp_sender_.TimeToSendRTCPReport()) {
		rtcp_sender_.SendRTCP(_get_feedback_state(), kRtcpReport);
	}
}

void RTCRtcpHandler::DeliverRtcp(const uint8_t* packet, size_t length) {
	rtcp_receiver_.IncomingPacket(packet, length);
}

void RTCRtcpHandler::SetSendingStatus(bool enabled) {
	{
		MutexLock lock(&send_mutex_);
		if (sending_media_ == enabled) {
			return;
		}
		sending_media_ = enabled;
		RTC_LOG(LS_VERBOSE) << "[" << this << "] RTCRtcpHandler::SetSendingStatus enabled " << enabled << (is_video_ ? " video " : " audio ");
	}

	rtcp_sender_.SetSendingStatus(_get_feedback_state(), enabled);
}

void RTCRtcpHandler::SetLastRtpTime(
	uint32_t rtp_timestamp, int64_t capture_time_ms, int8_t payload_type /* = -1 */) {
	rtcp_sender_.SetLastRtpTime(rtp_timestamp, capture_time_ms, payload_type);
}

absl::optional<TimeDelta> RTCRtcpHandler::PeriodicUpdate() {
	Timestamp check_since = clock_->CurrentTime() - kRttUpdateInterval;
	absl::optional<TimeDelta> rtt =
		rtcp_receiver_.OnPeriodicRttUpdate(check_since, rtcp_sender_.Sending());

	if (rtt) {
		rtt_stats_->OnRttUpdate(rtt->ms());
		set_rtt_ms(rtt->ms());
	}

	if (TMMBR() && rtcp_receiver_.UpdateTmmbrTimers()) {
		rtcp_receiver_.NotifyTmmbrUpdated();
	}

	return rtt;
}

void RTCRtcpHandler::CheckTMMBR() {
	if (rtcp_sender_.TMMBR() && rtcp_receiver_.UpdateTmmbrTimers())
		rtcp_receiver_.NotifyTmmbrUpdated();
}

void RTCRtcpHandler::SetRtt(int64_t rtt_ms) {
	packet_history_.SetRtt(rtt_ms);
}

int32_t RTCRtcpHandler::SendRTCP(RTCPPacketType packet_type) {
	return rtcp_sender_.SendRTCP(_get_feedback_state(), packet_type);
}

int32_t RTCRtcpHandler::RemoteRTCPStat(
	std::vector<RTCPReportBlock>* receive_blocks) const {
	return rtcp_receiver_.StatisticsReceived(receive_blocks);
}

int32_t RTCRtcpHandler::RTT(uint32_t remote_ssrc,
	int64_t* rtt,
	int64_t* avg_rtt,
	int64_t* min_rtt,
	int64_t* max_rtt) const {
	int32_t ret = rtcp_receiver_.RTT(remote_ssrc, rtt, avg_rtt, min_rtt, max_rtt);
	if (rtt && *rtt == 0) {
		// Try to get RTT from RtcpRttStats class.
		*rtt = rtt_ms();
	}
	return ret;
}

void RTCRtcpHandler::PutRtpPacketInHistory(std::unique_ptr<RtpPacketToSend> packet) {
	const int64_t now_ms = clock_->TimeInMilliseconds();
	packet_history_.PutRtpPacket(std::move(packet), now_ms);
}

bool RTCRtcpHandler::StorePackets() const {
	return packet_history_.GetStorageMode() != RtpPacketHistory::StorageMode::kDisabled;
}

int32_t RTCRtcpHandler::ReSendPacket(uint16_t packet_id) {
	bool transmit_pending;
	return _resend_packet_in_history(packet_id, transmit_pending);
}

void RTCRtcpHandler::OnReceivedNack(const std::vector<uint16_t>& nack_sequence_numbers) {
	int64_t rtt = rtt_ms();

	if (is_video_) {
		if (rtt == 0) {
			rtcp_receiver_.RTT(rtcp_receiver_.RemoteSSRC(), NULL, &rtt, NULL, NULL);
		}

		packet_history_.SetRtt(5 + rtt);

		for (uint16_t seq_no : nack_sequence_numbers) {
			const int32_t bytes_sent = ReSendPacket(seq_no);
			if (bytes_sent < 0) {
				// Failed to send one Sequence number. Give up the rest in this nack.
				RTC_LOG(LS_WARNING) << "["<<this<<"] Failed resending RTP packet " << seq_no
					<< ", Discard rest of packets.";
				break;
			}
		}
	}
}

void RTCRtcpHandler::OnPacketFeedbackVector(
	std::vector<StreamFeedbackObserver::StreamPacketInfo>& packet_feedback_vector) {
	if (is_video_) {
		std::map<uint32_t, std::vector<uint16_t>> acked_packets_per_ssrc;
		for (const StreamFeedbackObserver::StreamPacketInfo& packet : packet_feedback_vector) {
			if (packet.received) {
				acked_packets_per_ssrc[packet.ssrc].push_back(packet.rtp_sequence_number);
			}
		}

		for (const auto& kv : acked_packets_per_ssrc) {
			rtc::ArrayView<const uint16_t> rtp_sequence_numbers(kv.second);
			packet_history_.CullAcknowledgedPackets(rtp_sequence_numbers);
		}
	}
}

void RTCRtcpHandler::SetRtxPayloadType(
	int payload_type, int associated_payload_type) {
	RTC_DCHECK_LE(payload_type, 127);
	RTC_DCHECK_LE(associated_payload_type, 127);

	if (payload_type < 0) {
		RTC_LOG(LS_ERROR) << "Invalid RTX payload type: " << payload_type << ".";
		return;
	}

	rtx_payload_type_map_[associated_payload_type] = payload_type;
}

RTCPSender::FeedbackState RTCRtcpHandler::_get_feedback_state() {
	RTCPSender::FeedbackState state;

	state.receiver = &rtcp_receiver_;

	_last_received_NTP(&state.last_rr_ntp_secs, &state.last_rr_ntp_frac,
		&state.remote_sr);

	state.last_xr_rtis = rtcp_receiver_.ConsumeReceivedXrReferenceTimeInfo();

	return state;
}

bool RTCRtcpHandler::_last_received_NTP(
	uint32_t* rtcp_arrival_time_secs,
	uint32_t* rtcp_arrival_time_frac,
	uint32_t* remote_sr) const {
	uint32_t ntp_secs = 0;
	uint32_t ntp_frac = 0;

	if (!rtcp_receiver_.NTP(&ntp_secs, &ntp_frac, rtcp_arrival_time_secs,
		rtcp_arrival_time_frac, NULL)) {
		return false;
	}
	*remote_sr =
		((ntp_secs & 0x0000ffff) << 16) + ((ntp_frac & 0xffff0000) >> 16);
	return true;
}

int32_t RTCRtcpHandler::_resend_packet_in_history(uint16_t packet_id, bool &transmit_pending) {
	transmit_pending = false;
	absl::optional<RtpPacketHistory::PacketState> stored_packet =
		packet_history_.GetPacketState(packet_id);

	if (!stored_packet) {
		return 0;
	}

	if (stored_packet->pending_transmission) {
		transmit_pending = true;
		return 0;
	}

	const int32_t packet_size = static_cast<int32_t>(stored_packet->packet_size);

	std::unique_ptr<RtpPacketToSend> packet =
		packet_history_.GetPacketAndMarkAsPending(
		packet_id, [&](const RtpPacketToSend& stored_packet) {

		std::unique_ptr<RtpPacketToSend> retransmit_packet = BuildRtxPacket(stored_packet);
		if (retransmit_packet) {
			retransmit_packet->set_retransmitted_sequence_number(
				stored_packet.SequenceNumber());
		}
		return retransmit_packet;
	});

	if (!packet) {
		return -1;
	}
	packet->set_packet_type(RtpPacketMediaType::kRetransmission);
	packet->set_fec_protect_packet(false);

	send_rtx_packet(std::move(packet));

	return packet_size;
}

static void CopyHeaderAndExtensionsToRtxPacket(const RtpPacketToSend& packet,
	RtpPacketToSend* rtx_packet) {
	rtx_packet->SetMarker(packet.Marker());
	rtx_packet->SetTimestamp(packet.Timestamp());

	const std::vector<uint32_t> csrcs = packet.Csrcs();
	rtx_packet->SetCsrcs(csrcs);

	for (int extension_num = kRtpExtensionNone + 1;
		extension_num < kRtpExtensionNumberOfExtensions; ++extension_num) {
		auto extension = static_cast<RTPExtensionType>(extension_num);

		if (extension == kRtpExtensionMid ||
			extension == kRtpExtensionRtpStreamId) {
			continue;
		}

		if (!packet.HasExtension(extension)) {
			continue;
		}

		rtc::ArrayView<const uint8_t> source = packet.FindExtension(extension);

		rtc::ArrayView<uint8_t> destination =
			rtx_packet->AllocateExtension(extension, source.size());

		if (destination.empty() || source.size() != destination.size()) {
			continue;
		}

		std::memcpy(destination.begin(), source.begin(), destination.size());
	}
}

std::unique_ptr<RtpPacketToSend> RTCRtcpHandler::BuildRtxPacket(
	const RtpPacketToSend& packet) {
	uint16_t seq;
	{
		MutexLock lock(&send_mutex_);
		if (!sending_media_)
			return nullptr;
		seq = sequence_number_rtx_++;
	}

	auto kv = rtx_payload_type_map_.find(packet.PayloadType());
	if (kv == rtx_payload_type_map_.end())
		return nullptr;

	std::unique_ptr<RtpPacketToSend> rtx_packet;

	rtx_packet = std::make_unique<RtpPacketToSend>(&header_extension_map_,
		max_packet_size_);
	rtx_packet->SetPayloadType(kv->second);
	rtx_packet->SetSequenceNumber(seq);
	rtx_packet->SetSsrc(*rtx_ssrc_);

	CopyHeaderAndExtensionsToRtxPacket(packet, rtx_packet.get());

	uint8_t* rtx_payload =
		rtx_packet->AllocatePayload(packet.payload_size() + kRtxHeaderSize);
	if (rtx_payload == nullptr)
		return nullptr;

	ByteWriter<uint16_t>::WriteBigEndian(rtx_payload, packet.SequenceNumber());

	auto payload = packet.payload();
	memcpy(rtx_payload + kRtxHeaderSize, payload.data(), payload.size());

	rtx_packet->set_application_data(packet.application_data());
	rtx_packet->set_additional_data(packet.additional_data());

	rtx_packet->set_capture_time_ms(packet.capture_time_ms());

	return rtx_packet;
}

void RTCRtcpHandler::send_rtx_packet(
	std::unique_ptr<RtpPacketToSend> packet, bool is_rtx_fw /* = false */) {
	int64_t now_ms = clock_->TimeInMilliseconds();

	PacketOptions options;
	options.is_retransmit = true;
	uint64_t transprot_sequence_number;

	packet->SetExtension<AbsoluteSendTime>(AbsoluteSendTime::MsTo24Bits(now_ms));

	if (packet->HasExtension(kRtpExtensionTransportSequenceNumber)) {
		transprot_sequence_number = transport_seq_generator_->GetTransportSequenceNumber();
		options.packet_id = (transprot_sequence_number & 0xFFFF);
		packet->SetExtension<TransportSequenceNumber>(options.packet_id);
	}

	{
		MutexLock lock(&send_mutex_);
		if (!sending_media_) {
			return;
		}

		send_transport_->SendRtp(packet->data(), packet->size(), options);
	}

	packet_history_.MarkPacketAsSent(*packet->retransmitted_sequence_number());
}

bool RTCRtcpHandler::TMMBR() const {
	return rtcp_sender_.TMMBR();
}

void RTCRtcpHandler::SetTmmbn(std::vector<rtcp::TmmbItem> bounding_set) {
	rtcp_sender_.SetTmmbn(std::move(bounding_set));
}

static void _dump_kimi_rtp_header_extension(RTPHeaderExtension extension) {
	RTC_LOG(LS_INFO) << "[[[[ _dump_rtp_header_extension";
	RTC_LOG(LS_INFO) << "extension hasTransmissionTimeOffset " << extension.hasTransmissionTimeOffset;
	RTC_LOG(LS_INFO) << "extension hasAbsoluteSendTime " << extension.hasAbsoluteSendTime;
	RTC_LOG(LS_INFO) << "extension hasTransportSequenceNumber " << extension.hasTransportSequenceNumber;
	RTC_LOG(LS_INFO) << "extension hasAudioLevel " << extension.hasAudioLevel;
	RTC_LOG(LS_INFO) << "extension hasVideoRotation " << extension.hasVideoRotation;
	RTC_LOG(LS_INFO) << "extension has_video_timing " << extension.has_video_timing;
	RTC_LOG(LS_INFO) << "extension playout_delay min_ms " << extension.playout_delay.min_ms << " max_ms " << extension.playout_delay.max_ms;
	RTC_LOG(LS_INFO) << "extension stream_id " << extension.stream_id.c_str();
	RTC_LOG(LS_INFO) << "extension repaired_stream_id " << extension.repaired_stream_id.c_str();
	RTC_LOG(LS_INFO) << "extension mid " << extension.mid.c_str();
	RTC_LOG(LS_INFO) << "_dump_rtp_header_extension ]]]]";
}

std::unique_ptr<RtpPacketToSend> RTCRtcpHandler::RtpPacketReceivedToSend(
	const RtpPacketReceived *received_packet, RtpHeaderExtensionMap& header_extension_map, bool isVideo /* = true */) {

	std::unique_ptr<RtpPacketToSend> send_packet =
		std::make_unique<RtpPacketToSend>(&header_extension_map, IP_PACKET_SIZE - 28);

	send_packet->SetPayloadType(received_packet->PayloadType());
	send_packet->SetSequenceNumber(received_packet->SequenceNumber());
	send_packet->SetSsrc(received_packet->Ssrc());
	send_packet->SetMarker(received_packet->Marker());
	send_packet->SetTimestamp(received_packet->Timestamp());
	const std::vector<uint32_t> csrcs = received_packet->Csrcs();
	send_packet->SetCsrcs(csrcs);
	
	bool is_set_mid = false;
	if (!received_packet->get_mid().empty()) {
		const std::string mid = received_packet->get_mid();
		bool is_set_ext = send_packet->SetExtension<RtpMid>(mid);
		is_set_mid = true;
	}

	for (int extension_num = kRtpExtensionNone + 1;
		extension_num < kRtpExtensionNumberOfExtensions; ++extension_num) {
		auto extension = static_cast<RTPExtensionType>(extension_num);

		if ((extension == kRtpExtensionMid && is_set_mid) ||
			extension == kRtpExtensionRtpStreamId) {
			continue;
		}

		if (!received_packet->HasExtension(extension)) {
			continue;
		}

		rtc::ArrayView<const uint8_t> source = received_packet->FindExtension(extension);

		rtc::ArrayView<uint8_t> destination =
			send_packet->AllocateExtension(extension, source.size());

		if (destination.empty() || source.size() != destination.size()) {
			continue;
		}

		std::memcpy(destination.begin(), source.begin(), destination.size());
	}

	uint8_t* send_payload = send_packet->AllocatePayload(received_packet->payload_size());
	auto received_payload = received_packet->payload();
	memcpy(send_payload, received_payload.data(), received_payload.size());

	if (isVideo) {
		send_packet->set_packet_type(RtpPacketMediaType::kVideo);
		send_packet->set_is_key_frame(received_packet->get_key_frame());
	}
	else {
		send_packet->set_packet_type(RtpPacketMediaType::kAudio);
	}

	return std::move(send_packet);
}

void RTCRtcpHandler::set_rtt_ms(int64_t rtt_ms) {
	{
		MutexLock lock(&mutex_rtt_);
		rtt_ms_ = rtt_ms;
	}

	packet_history_.SetRtt(rtt_ms);
}

int64_t RTCRtcpHandler::rtt_ms() const {
	MutexLock lock(&mutex_rtt_);
	return rtt_ms_;
}

void RTCRtpRtcpSendImpl::MaybeSendRtcp() {
	RTC_DCHECK_RUN_ON(worker_queue_);
	if (rtcp_sender_.TimeToSendRTCPReport())
		rtcp_sender_.SendRTCP(GetFeedbackState(), kRtcpReport);
}

void RTCRtcpHandler::MaybeSendRtcpAtOrAfterTimestamp(
	Timestamp execution_time) {
	RTC_DCHECK_RUN_ON(worker_queue_);
	Timestamp now = clock_->CurrentTime();
	if (now >= execution_time) {
		MaybeSendRtcp();
		return;
	}

	TimeDelta delta = execution_time - now;
	// TaskQueue may run task 1ms earlier, so don't print warning if in this case.
	if (delta > TimeDelta::Millis(1)) {
		RTC_DLOG(LS_WARNING) << "BUGBUG: Task queue scheduled delayed call "
			<< delta.ms() << " too early.";
	}

	ScheduleMaybeSendRtcpAtOrAfterTimestamp(execution_time, delta);
}

void RTCRtcpHandler::ScheduleRtcpSendEvaluation(TimeDelta duration) {
	// We end up here under various sequences including the worker queue, and
	// the RTCPSender lock is held.
	// We're assuming that the fact that RTCPSender executes under other sequences
	// than the worker queue on which it's created on implies that external
	// synchronization is present and removes this activity before destruction.
	if (duration.IsZero()) {
		worker_queue_->PostTask(SafeTask(task_safety_.flag(), [this] {
			RTC_DCHECK_RUN_ON(worker_queue_);
			MaybeSendRtcp();
		}));
	}
	else {
		Timestamp execution_time = clock_->CurrentTime() + duration;
		ScheduleMaybeSendRtcpAtOrAfterTimestamp(execution_time, duration);
	}
}

void RTCRtcpHandler::ScheduleMaybeSendRtcpAtOrAfterTimestamp(
	Timestamp execution_time,
	TimeDelta duration) {
	// We end up here under various sequences including the worker queue, and
	// the RTCPSender lock is held.
	// See note in ScheduleRtcpSendEvaluation about why `worker_queue_` can be
	// accessed.
	worker_queue_->PostDelayedTask(
		SafeTask(task_safety_.flag(),
		[this, execution_time] {
		RTC_DCHECK_RUN_ON(worker_queue_);
		MaybeSendRtcpAtOrAfterTimestamp(execution_time);
	}),
		duration.RoundUpTo(TimeDelta::Millis(1)));
}

}
