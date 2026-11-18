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
 *  module/pacing/packet_router.h.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#ifndef __RTC_MODULE_PACING_RTC_PACKET_ROUTER_H__
#define __RTC_MODULE_PACING_RTC_PACKET_ROUTER_H__

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "api/transport/network_types.h"
#include "modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "modules/rtp_rtcp/source/rtcp_packet.h"
#include "modules/rtp_rtcp/source/rtp_packet_to_send.h"
#include "rtc_base/synchronization/sequence_checker.h"

#include "RTCPacingController.h"

#include "../rtc_pc_src/_deprecate_defines.h"

namespace webrtc {

class RTCRtpRtcpInterface;

class RTCPacketRouter : public RTCPacingController::RTCPacketSender {
public:
	RTCPacketRouter();
	explicit RTCPacketRouter(uint16_t start_transport_seq);
	~RTCPacketRouter() override;

	void AddSendRtpModule(RTCRtpRtcpInterface* rtp_module, bool remb_candidate);
	void RemoveSendRtpModule(RTCRtpRtcpInterface* rtp_module);

	void AddReceiveRtpModule(RtcpFeedbackSenderInterface* rtcp_sender,
		bool remb_candidate);
	void RemoveReceiveRtpModule(RtcpFeedbackSenderInterface* rtcp_sender);

	//////////////////////////////////////////////////////////////////////////
	// RTCPacingController::RTCPacketSender method
	void SendPacket(std::unique_ptr<RtpPacketToSend> packet,
		const PacedPacketInfo& cluster_info) override;
	std::vector<std::unique_ptr<RtpPacketToSend>> FetchFec() override;
	std::vector<std::unique_ptr<RtpPacketToSend>> GeneratePadding(
		DataSize size) override;
	void OnAbortedRetransmissions(
		uint32_t ssrc,
		rtc::ArrayView<const uint16_t> sequence_numbers) override;
	absl::optional<uint32_t> GetRtxSsrcForMedia(uint32_t ssrc) const override;
	void OnBatchComplete() override;
	//////////////////////////////////////////////////////////////////////////

	uint16_t CurrentTransportSequenceNumber() const;

	// Send REMB feedback.
	void SendRemb(int64_t bitrate_bps, std::vector<uint32_t> ssrcs);

	// Sends `packets` in one or more IP packets.
	void SendCombinedRtcpPacket(
		std::vector<std::unique_ptr<rtcp::RtcpPacket>> packets);

private:
	void AddRembModuleCandidate(RtcpFeedbackSenderInterface* candidate_module,
		bool media_sender);
	void MaybeRemoveRembModuleCandidate(RtcpFeedbackSenderInterface* candidate_module,
		bool media_sender);
	void UnsetActiveRembModule();
	void DetermineActiveRembModule();
	void AddSendRtpModuleToMap(RTCRtpRtcpInterface* rtp_module, uint32_t ssrc);
	void RemoveSendRtpModuleFromMap(uint32_t ssrc);

private:
	SequenceChecker thread_checker_;
	// Ssrc to RtpRtcpInterface module;
	std::unordered_map<uint32_t, RTCRtpRtcpInterface*> send_modules_map_
		RTC_GUARDED_BY(thread_checker_);
	std::list<RTCRtpRtcpInterface*> send_modules_list_
		RTC_GUARDED_BY(thread_checker_);
	// The last module used to send media.
	RTCRtpRtcpInterface* last_send_module_ RTC_GUARDED_BY(thread_checker_);
	// Rtcp modules of the rtp receivers.
	std::vector<RtcpFeedbackSenderInterface*> rtcp_feedback_senders_
		RTC_GUARDED_BY(thread_checker_);

	std::vector<RtcpFeedbackSenderInterface*> sender_remb_candidates_
		RTC_GUARDED_BY(thread_checker_);
	std::vector<RtcpFeedbackSenderInterface*> receiver_remb_candidates_
		RTC_GUARDED_BY(thread_checker_);
	RtcpFeedbackSenderInterface* active_remb_module_
		RTC_GUARDED_BY(thread_checker_);

	uint64_t transport_seq_ RTC_GUARDED_BY(thread_checker_);

// thinking... TODO... current disable
	std::vector<std::unique_ptr<RtpPacketToSend>> pending_fec_packets_
		RTC_GUARDED_BY(thread_checker_);


	std::set<RTCRtpRtcpInterface*> modules_used_in_current_batch_
		RTC_GUARDED_BY(thread_checker_);

private:
	RTC_DISALLOW_COPY_AND_ASSIGN(RTCPacketRouter);
};

}

#endif // __RTC_MODULE_PACING_RTC_PACKET_ROUTER_H__