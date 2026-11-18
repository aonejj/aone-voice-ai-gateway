/*
*  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
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
 *  media/engine/webrtc_video_engine.cc.
 *
 *  Distributed under the same BSD-style license as the original
 *  WebRTC source code.
 */

#include <stdio.h>

#include <algorithm>
#include <set>
#include <string>
#include <utility>

#include "absl/algorithm/container.h"
#include "absl/strings/match.h"

#include "api/rtc_error.h"
#include "rtc_base/logging.h"
#include "media/base/rtp_utils.h"
#include "media/engine/constants.h"

#include "_webrtc_media_engine.h"

namespace cricket {

void AddDefaultFeedbackParams(VideoCodec* codec) {
	// Don't add any feedback params for RED and ULPFEC.
	if (codec->name == kRedCodecName || codec->name == kUlpfecCodecName)
		return;
	codec->AddFeedbackParam(FeedbackParam(kRtcpFbParamRemb, kParamValueEmpty));
	codec->AddFeedbackParam(
		FeedbackParam(kRtcpFbParamTransportCc, kParamValueEmpty));
	// Don't add any more feedback params for FLEXFEC.
	if (codec->name == kFlexfecCodecName)
		return;
	codec->AddFeedbackParam(FeedbackParam(kRtcpFbParamCcm, kRtcpFbCcmParamFir));
	codec->AddFeedbackParam(FeedbackParam(kRtcpFbParamNack, kParamValueEmpty));
	codec->AddFeedbackParam(FeedbackParam(kRtcpFbParamNack, kRtcpFbNackParamPli));
	if (codec->name == kVp8CodecName /* &&
		IsEnabled(trials, "WebRTC-RtcpLossNotification") */) {
		codec->AddFeedbackParam(FeedbackParam(kRtcpFbParamLntf, kParamValueEmpty));
	}
}

bool IsTemporalLayersSupported(const std::string& codec_name) {
	return absl::EqualsIgnoreCase(codec_name, kVp8CodecName) ||
		absl::EqualsIgnoreCase(codec_name, kVp9CodecName);
}

static std::string CodecVectorToString(const std::vector<VideoCodec>& codecs) {
	rtc::StringBuilder out;
	out << "{";
	for (size_t i = 0; i < codecs.size(); ++i) {
		out << codecs[i].ToString();
		if (i != codecs.size() - 1) {
			out << ", ";
		}
	}
	out << "}";
	return out.Release();
}

static bool ValidateCodecFormats(const std::vector<VideoCodec>& codecs) {
	bool has_video = false;
	for (size_t i = 0; i < codecs.size(); ++i) {
		if (!codecs[i].ValidateCodecFormat()) {
			return false;
		}
		if (codecs[i].GetCodecType() == VideoCodec::CODEC_VIDEO) {
			has_video = true;
		}
	}
	if (!has_video) {
		RTC_LOG(LS_ERROR) << "Setting codecs without a video codec is invalid: "
			<< CodecVectorToString(codecs);
		return false;
	}
	return true;
}

static bool ValidateStreamParams(const StreamParams& sp) {
	if (sp.ssrcs.empty()) {
		RTC_LOG(LS_ERROR) << "No SSRCs in stream parameters: " << sp.ToString();
		return false;
	}

	std::vector<uint32_t> primary_ssrcs;
	sp.GetPrimarySsrcs(&primary_ssrcs);
	std::vector<uint32_t> rtx_ssrcs;
	sp.GetFidSsrcs(primary_ssrcs, &rtx_ssrcs);
	for (uint32_t rtx_ssrc : rtx_ssrcs) {
		bool rtx_ssrc_present = false;
		for (uint32_t sp_ssrc : sp.ssrcs) {
			if (sp_ssrc == rtx_ssrc) {
				rtx_ssrc_present = true;
				break;
			}
		}
		if (!rtx_ssrc_present) {
			RTC_LOG(LS_ERROR) << "RTX SSRC '" << rtx_ssrc
				<< "' missing from StreamParams ssrcs: "
				<< sp.ToString();
			return false;
		}
	}
	if (!rtx_ssrcs.empty() && primary_ssrcs.size() != rtx_ssrcs.size()) {
		RTC_LOG(LS_ERROR)
			<< "RTX SSRCs exist, but don't cover all SSRCs (unsupported): "
			<< sp.ToString();
		return false;
	}

	return true;
}


std::vector<VideoCodec> GetPayloadTypesAndDefaultCodecs() {
	return{};
}

DefaultUnsignalledSsrcHandler::DefaultUnsignalledSsrcHandler() {}

UnsignalledSsrcHandler::Action DefaultUnsignalledSsrcHandler::OnUnsignalledSsrc(
	WebRtcVideoChannel* channel,
	uint32_t ssrc) {
	absl::optional<uint32_t> default_recv_ssrc =
		channel->GetDefaultReceiveStreamSsrc();

	if (default_recv_ssrc) {
		RTC_LOG(LS_INFO) << "Destroying old default receive stream for SSRC="
			<< ssrc << ".";
		channel->RemoveRecvStream(*default_recv_ssrc);
	}

	StreamParams sp = channel->unsignaled_stream_params();
	sp.ssrcs.push_back(ssrc);

	RTC_LOG(LS_INFO) << "Creating default receive stream for SSRC=" << ssrc
		<< ".";
	if (!channel->AddRecvStream(sp, /*default_stream=*/true)) {
		RTC_LOG(LS_WARNING) << "Could not create default receive stream.";
	}

	// SSRC 0 returns default_recv_base_minimum_delay_ms.
	const int unsignaled_ssrc = 0;
	int default_recv_base_minimum_delay_ms =
		channel->GetBaseMinimumPlayoutDelayMs(unsignaled_ssrc).value_or(0);
	// Set base minimum delay if it was set before for the default receive stream.
	channel->SetBaseMinimumPlayoutDelayMs(ssrc,
		default_recv_base_minimum_delay_ms);
	return kDeliverPacket;
}

WebRtcVideoEngine::WebRtcVideoEngine() {
	RTC_LOG(LS_INFO) << "WebRtcVideoEngine::WebRtcVideoEngine()";
}

WebRtcVideoEngine::~WebRtcVideoEngine() {
	RTC_LOG(LS_INFO) << "WebRtcVideoEngine::~WebRtcVideoEngine";
}

VideoMediaChannel* WebRtcVideoEngine::CreateMediaChannel(
	webrtc::RTCCall* call,
	const MediaConfig& config,
	const webrtc::CryptoOptions& crypto_options) {
	return new WebRtcVideoChannel(call, config, crypto_options);
}

std::vector<VideoCodec> WebRtcVideoEngine::send_codecs() const {
	return{};
}

std::vector<VideoCodec> WebRtcVideoEngine::recv_codecs() const {
	return{};
}

std::vector<webrtc::RtpHeaderExtensionCapability>
WebRtcVideoEngine::GetRtpHeaderExtensions() const {
	return{};
}

WebRtcVideoChannel::WebRtcVideoChannel(
	webrtc::RTCCall* call,
	const MediaConfig& config,
	const webrtc::CryptoOptions& crypto_options)
	: VideoMediaChannel(config),
	 worker_thread_(rtc::Thread::Current()),
	 call_(call),
	 unsignalled_ssrc_handler_(&default_unsignalled_ssrc_handler_), 
	 discard_unknown_ssrc_packets_(true), 
	 crypto_options_(crypto_options),
	 unknown_ssrc_packet_buffer_(new UnhandledPacketsBuffer()) {

	RTC_DCHECK(thread_checker_.IsCurrent());
	sending_ = false;

	recv_codecs_ = MapCodecs(GetPayloadTypesAndDefaultCodecs());

	recv_flexfec_payload_type_ =
		recv_codecs_.empty() ? 0 : recv_codecs_.front().flexfec_payload_type;

}

WebRtcVideoChannel::~WebRtcVideoChannel() {
	for (auto& kv : send_streams_)
		delete kv.second;
	for (auto& kv : receive_streams_)
		delete kv.second;
}

bool WebRtcVideoChannel::SetSendParameters(const VideoSendParameters& params) {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	RTC_LOG(LS_INFO) << "SetSendParameters: " << params.ToString();

	send_params_ = params;

	return true;
}

webrtc::RtpParameters WebRtcVideoChannel::GetRtpSendParameters(
	uint32_t ssrc) const {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	auto it = send_streams_.find(ssrc);
	if (it == send_streams_.end()) {
		RTC_LOG(LS_WARNING) << "Attempting to get RTP send parameters for stream "
			"with ssrc "
			<< ssrc << " which doesn't exist.";
		return webrtc::RtpParameters();
	}

	return{};
}

webrtc::RTCError WebRtcVideoChannel::SetRtpSendParameters(
	uint32_t ssrc,
	const webrtc::RtpParameters& parameters) {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	auto it = send_streams_.find(ssrc);
	if (it == send_streams_.end()) {
		RTC_LOG(LS_ERROR) << "Attempting to set RTP send parameters for stream "
			"with ssrc "
			<< ssrc << " which doesn't exist.";
		return webrtc::RTCError(webrtc::RTCErrorType::INTERNAL_ERROR);
	}

	// TODO(deadbeef): Handle setting parameters with a list of codecs in a
	// different order (which should change the send codec).
	webrtc::RtpParameters current_parameters = GetRtpSendParameters(ssrc);
	if (current_parameters.codecs != parameters.codecs) {
		RTC_DLOG(LS_ERROR) << "Using SetParameters to change the set of codecs "
			"is not currently supported.";
		return webrtc::RTCError(webrtc::RTCErrorType::INTERNAL_ERROR);
	}

	return webrtc::RTCError::OK();
}

webrtc::RtpParameters WebRtcVideoChannel::GetRtpReceiveParameters(
	uint32_t ssrc) const {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	webrtc::RtpParameters rtp_params;
	auto it = receive_streams_.find(ssrc);
	if (it == receive_streams_.end()) {
		RTC_LOG(LS_WARNING)
			<< "Attempting to get RTP receive parameters for stream "
			"with SSRC "
			<< ssrc << " which doesn't exist.";
		return webrtc::RtpParameters();
	}

	// Add codecs, which any stream is prepared to receive.
	for (const VideoCodec& codec : recv_params_.codecs) {
		rtp_params.codecs.push_back(codec.ToCodecParameters());
	}

	return rtp_params;
}

webrtc::RtpParameters WebRtcVideoChannel::GetDefaultRtpReceiveParameters()
const {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	webrtc::RtpParameters rtp_params;

	rtp_params.encodings.emplace_back();

	// Add codecs, which any stream is prepared to receive.
	for (const VideoCodec& codec : recv_params_.codecs) {
		rtp_params.codecs.push_back(codec.ToCodecParameters());
	}

	return rtp_params;
}

bool WebRtcVideoChannel::NonFlexfecReceiveCodecsHaveChanged(
	std::vector<VideoCodecSettings> before,
	std::vector<VideoCodecSettings> after) {
	// The receive codec order doesn't matter, so we sort the codecs before
	// comparing. This is necessary because currently the
	// only way to change the send codec is to munge SDP, which causes
	// the receive codec list to change order, which causes the streams
	// to be recreates which causes a "blink" of black video.  In order
	// to support munging the SDP in this way without recreating receive
	// streams, we ignore the order of the received codecs so that
	// changing the order doesn't cause this "blink".
	auto comparison = [](const VideoCodecSettings& codec1,
		const VideoCodecSettings& codec2) {
		return codec1.codec.id > codec2.codec.id;
	};
	absl::c_sort(before, comparison);
	absl::c_sort(after, comparison);

	// Changes in FlexFEC payload type are handled separately in
	// WebRtcVideoChannel::GetChangedRecvParameters, so disregard FlexFEC in the
	// comparison here.
	return !absl::c_equal(before, after,
		VideoCodecSettings::EqualsDisregardingFlexfec);
}

bool WebRtcVideoChannel::GetChangedRecvParameters(
	const VideoRecvParameters& params,
	ChangedRecvParameters* changed_params) const {
	if (!ValidateCodecFormats(params.codecs) ||
		!ValidateRtpExtensions(params.extensions)) {
		return false;
	}

	// Handle receive codecs.
	const std::vector<VideoCodecSettings> mapped_codecs =
		MapCodecs(params.codecs);
	if (mapped_codecs.empty()) {
		RTC_LOG(LS_ERROR) << "SetRecvParameters called without any video codecs.";
		return false;
	}

	// Verify that every mapped codec is supported locally.
	if (params.is_stream_active) {
		const std::vector<VideoCodec> local_supported_codecs;
		for (const VideoCodecSettings& mapped_codec : mapped_codecs) {
			if (!FindMatchingCodec(local_supported_codecs, mapped_codec.codec)) {
				RTC_LOG(LS_ERROR)
					<< "SetRecvParameters called with unsupported video codec: "
					<< mapped_codec.codec.ToString();
				return false;
			}
		}
	}

	if (NonFlexfecReceiveCodecsHaveChanged(recv_codecs_, mapped_codecs)) {
		changed_params->codec_settings =
			absl::optional<std::vector<VideoCodecSettings>>(mapped_codecs);
	}

	// Handle RTP header extensions.
	std::vector<webrtc::RtpExtension> filtered_extensions = FilterRtpExtensions(
		params.extensions, webrtc::RtpExtension::IsSupportedForVideo, false);
	if (filtered_extensions != recv_rtp_extensions_) {
		changed_params->rtp_header_extensions =
			absl::optional<std::vector<webrtc::RtpExtension>>(filtered_extensions);
	}

	int flexfec_payload_type = mapped_codecs.front().flexfec_payload_type;
	if (flexfec_payload_type != recv_flexfec_payload_type_) {
		changed_params->flexfec_payload_type = flexfec_payload_type;
	}

	return true;
}

bool WebRtcVideoChannel::SetRecvParameters(const VideoRecvParameters& params) {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	RTC_LOG(LS_INFO) << "SetRecvParameters: " << params.ToString();
	ChangedRecvParameters changed_params;
	if (!GetChangedRecvParameters(params, &changed_params)) {
		return false;
	}
	if (changed_params.flexfec_payload_type) {
		RTC_LOG(LS_INFO) << "Changing FlexFEC payload type (recv) from "
			<< recv_flexfec_payload_type_ << " to "
			<< *changed_params.flexfec_payload_type;
		recv_flexfec_payload_type_ = *changed_params.flexfec_payload_type;
	}
	if (changed_params.rtp_header_extensions) {
		recv_rtp_extensions_ = *changed_params.rtp_header_extensions;
	}
	if (changed_params.codec_settings) {
		RTC_LOG(LS_INFO) << "Changing recv codecs from "
			<< CodecSettingsVectorToString(recv_codecs_) << " to "
			<< CodecSettingsVectorToString(
			*changed_params.codec_settings);
		recv_codecs_ = *changed_params.codec_settings;
	}

	recv_params_ = params;
	return true;
}

std::string WebRtcVideoChannel::CodecSettingsVectorToString(
	const std::vector<VideoCodecSettings>& codecs) {
	rtc::StringBuilder out;
	out << "{";
	for (size_t i = 0; i < codecs.size(); ++i) {
		out << codecs[i].codec.ToString();
		if (i != codecs.size() - 1) {
			out << ", ";
		}
	}
	out << "}";
	return out.Release();
}

bool WebRtcVideoChannel::GetSendCodec(VideoCodec* codec) {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	if (!send_codec_) {
		RTC_LOG(LS_VERBOSE) << "GetSendCodec: No send codec set.";
		return false;
	}
	*codec = send_codec_->codec;
	return true;
}

bool WebRtcVideoChannel::SetSend(bool send) {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	RTC_LOG(LS_VERBOSE) << "SetSend: " << (send ? "true" : "false");
	if (send && !send_codec_) {
		RTC_DLOG(LS_ERROR) << "SetSend(true) called before setting codec.";
		return false;
	}

	sending_ = send;
	return true;
}

bool WebRtcVideoChannel::SetVideoSend(
	uint32_t ssrc) {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	RTC_DCHECK(ssrc != 0);
	RTC_LOG(LS_INFO) << "SetVideoSend (ssrc= " << ssrc;

	const auto& kv = send_streams_.find(ssrc);
	if (kv == send_streams_.end()) {
		// Allow unknown ssrc only if source is null.
		RTC_LOG(LS_ERROR) << "No sending stream on ssrc " << ssrc;
		return false;
	}
	return false;
}

bool WebRtcVideoChannel::ValidateSendSsrcAvailability(
	const StreamParams& sp) const {
	for (uint32_t ssrc : sp.ssrcs) {
		if (send_ssrcs_.find(ssrc) != send_ssrcs_.end()) {
			RTC_LOG(LS_ERROR) << "Send stream with SSRC '" << ssrc
				<< "' already exists.";
			return false;
		}
	}
	return true;
}

bool WebRtcVideoChannel::ValidateReceiveSsrcAvailability(
	const StreamParams& sp) const {
	for (uint32_t ssrc : sp.ssrcs) {
		if (receive_ssrcs_.find(ssrc) != receive_ssrcs_.end()) {
			RTC_LOG(LS_ERROR) << "Receive stream with SSRC '" << ssrc
				<< "' already exists.";
			return false;
		}
	}
	return true;
}

bool WebRtcVideoChannel::AddSendStream(const StreamParams& sp) {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	RTC_LOG(LS_INFO) << "AddSendStream: " << sp.ToString();

	return false;
}

bool WebRtcVideoChannel::RemoveSendStream(uint32_t ssrc) {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	RTC_LOG(LS_INFO) << "RemoveSendStream: " << ssrc;

	return false;
}

void WebRtcVideoChannel::DeleteReceiveStream(
	WebRtcVideoChannel::WebRtcVideoReceiveStream* stream) {

	delete stream;
}

bool WebRtcVideoChannel::AddRecvStream(const StreamParams& sp) {
	return AddRecvStream(sp, false);
}

bool WebRtcVideoChannel::AddRecvStream(const StreamParams& sp,
	bool default_stream) {
	RTC_DCHECK_RUN_ON(&thread_checker_);

	RTC_LOG(LS_INFO) << "["<<this<<"] WebRtcVideoChannel::AddRecvStream"
		<< (default_stream ? " (default stream)" : "") << ": "
		<< sp.ToString();
	if (!sp.has_ssrcs()) {
		// This is a StreamParam with unsignaled SSRCs. Store it, so it can be used
		// later when we know the SSRC on the first packet arrival.
		unsignaled_stream_params_ = sp;
		return true;
	}

	if (!ValidateStreamParams(sp))
		return false;

	uint32_t ssrc = sp.first_ssrc();

	// Remove running stream if this was a default stream.
	const auto& prev_stream = receive_streams_.find(ssrc);
	if (prev_stream != receive_streams_.end()) {
		DeleteReceiveStream(prev_stream->second);
		receive_streams_.erase(prev_stream);
	}

	if (!ValidateReceiveSsrcAvailability(sp))
		return false;

	for (uint32_t used_ssrc : sp.ssrcs)
		receive_ssrcs_.insert(used_ssrc);

	return false;
}

bool WebRtcVideoChannel::RemoveRecvStream(uint32_t ssrc) {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	RTC_LOG(LS_INFO) << "RemoveRecvStream: " << ssrc;

	std::map<uint32_t, WebRtcVideoReceiveStream*>::iterator stream =
		receive_streams_.find(ssrc);
	if (stream == receive_streams_.end()) {
		RTC_LOG(LS_ERROR) << "Stream not found for ssrc: " << ssrc;
		return false;
	}
	DeleteReceiveStream(stream->second);
	receive_streams_.erase(stream);

	return true;
}

void WebRtcVideoChannel::ResetUnsignaledRecvStream() {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	RTC_LOG(LS_INFO) << "ResetUnsignaledRecvStream.";
	unsignaled_stream_params_ = StreamParams();

	// Delete any created default streams. This is needed to avoid SSRC collisions
	// in Call's RtpDemuxer, in the case that |this| has created a default video
	// receiver, and then some other WebRtcVideoChannel gets the SSRC signaled
	// in the corresponding Unified Plan "m=" section.
	auto it = receive_streams_.begin();
	while (it != receive_streams_.end()) {
		++it;
	}
}

void WebRtcVideoChannel::OnPacketReceived(rtc::CopyOnWriteBuffer packet,
	int64_t packet_time_us) {
	RTC_DCHECK_RUN_ON(&thread_checker_);
}

void WebRtcVideoChannel::BackfillBufferedPackets(
	rtc::ArrayView<const uint32_t> ssrcs) {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	if (!unknown_ssrc_packet_buffer_) {
		return;
	}

	int delivery_ok_cnt = 0;
	int delivery_unknown_ssrc_cnt = 0;
	int delivery_packet_error_cnt = 0;

	rtc::StringBuilder out;
	out << "[ ";
	for (uint32_t ssrc : ssrcs) {
		out << std::to_string(ssrc) << " ";
	}
	out << "]";
	auto level = rtc::LS_INFO;
	if (delivery_unknown_ssrc_cnt > 0 || delivery_packet_error_cnt > 0) {
		level = rtc::LS_ERROR;
	}
	int total =
		delivery_ok_cnt + delivery_unknown_ssrc_cnt + delivery_packet_error_cnt;
	RTC_LOG_V(level) << "Backfilled " << total
		<< " packets for ssrcs: " << out.Release()
		<< " ok: " << delivery_ok_cnt
		<< " error: " << delivery_packet_error_cnt
		<< " unknown: " << delivery_unknown_ssrc_cnt;
}

WebRtcVideoChannel::WebRtcVideoReceiveStream*
WebRtcVideoChannel::FindReceiveStream(uint32_t ssrc) {
	if (ssrc == 0) {
		absl::optional<uint32_t> default_ssrc = GetDefaultReceiveStreamSsrc();
		if (!default_ssrc) {
			return nullptr;
		}
		ssrc = *default_ssrc;
	}
	auto it = receive_streams_.find(ssrc);
	if (it != receive_streams_.end()) {
		return it->second;
	}
	return nullptr;
}

void WebRtcVideoChannel::GenerateKeyFrame(uint32_t ssrc) {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	WebRtcVideoReceiveStream* stream = FindReceiveStream(ssrc);
}

void WebRtcVideoChannel::OnReadyToSend(bool ready) {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	RTC_LOG(LS_VERBOSE) << "OnReadyToSend: " << (ready ? "Ready." : "Not ready.");
}

void WebRtcVideoChannel::OnNetworkRouteChanged(
	const std::string& transport_name,
	const rtc::NetworkRoute& network_route) {
	RTC_DCHECK_RUN_ON(&thread_checker_);

}

void WebRtcVideoChannel::SetInterface(NetworkInterface* iface) {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	MediaChannel::SetInterface(iface);
	// Set the RTP recv/send buffer to a bigger size.

	// The group should be a positive integer with an explicit size, in
	// which case that is used as UDP recevie buffer size. All other values shall
	// result in the default value being used.
	const std::string group_name_recv_buf_size;
	int recv_buffer_size = kVideoRtpRecvBufferSize;
	if (!group_name_recv_buf_size.empty() &&
		(sscanf(group_name_recv_buf_size.c_str(), "%d", &recv_buffer_size) != 1 ||
		recv_buffer_size <= 0)) {
		RTC_LOG(LS_WARNING) << "Invalid receive buffer size: "
			<< group_name_recv_buf_size;
		recv_buffer_size = kVideoRtpRecvBufferSize;
	}

	MediaChannel::SetOption(NetworkInterface::ST_RTP, rtc::Socket::OPT_RCVBUF,
		recv_buffer_size);

	// Speculative change to increase the outbound socket buffer size.
	// In b/15152257, we are seeing a significant number of packets discarded
	// due to lack of socket buffer space, although it's not yet clear what the
	// ideal value should be.

	const std::string group_name_send_buf_size;
	int send_buffer_size = kVideoRtpSendBufferSize;
	if (!group_name_send_buf_size.empty() &&
		(sscanf(group_name_send_buf_size.c_str(), "%d", &send_buffer_size) != 1 ||
		send_buffer_size <= 0)) {
		RTC_LOG(LS_WARNING) << "Invalid send buffer size: "
			<< group_name_send_buf_size;
		send_buffer_size = kVideoRtpSendBufferSize;
	}

	MediaChannel::SetOption(NetworkInterface::ST_RTP, rtc::Socket::OPT_SNDBUF,
		send_buffer_size);
}

absl::optional<uint32_t> WebRtcVideoChannel::GetDefaultReceiveStreamSsrc() {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	absl::optional<uint32_t> ssrc;

	return absl::nullopt;
}

std::vector<webrtc::RtpSource> WebRtcVideoChannel::GetSources(
	uint32_t ssrc) const {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	auto it = receive_streams_.find(ssrc);
	if (it == receive_streams_.end()) {
		// TODO(bugs.webrtc.org/9781): Investigate standard compliance
		// with sources for streams that has been removed.
		RTC_LOG(LS_ERROR) << "Attempting to get contributing sources for SSRC:"
			<< ssrc << " which doesn't exist.";
		return{};
	}

	return{};
}

bool WebRtcVideoChannel::SetBaseMinimumPlayoutDelayMs(uint32_t ssrc,
	int delay_ms) {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	absl::optional<uint32_t> default_ssrc = GetDefaultReceiveStreamSsrc();

	// SSRC of 0 represents the default receive stream.
	if (ssrc == 0) {
		default_recv_base_minimum_delay_ms_ = delay_ms;
	}

	if (ssrc == 0 && !default_ssrc) {
		return true;
	}

	if (ssrc == 0 && default_ssrc) {
		ssrc = default_ssrc.value();
	}

	auto stream = receive_streams_.find(ssrc);
	if (stream != receive_streams_.end()) {
		return true;
	}
	else {
		RTC_LOG(LS_ERROR) << "No stream found to set base minimum playout delay";
		return false;
	}
}

absl::optional<int> WebRtcVideoChannel::GetBaseMinimumPlayoutDelayMs(
	uint32_t ssrc) const {
	RTC_DCHECK_RUN_ON(&thread_checker_);
	// SSRC of 0 represents the default receive stream.
	if (ssrc == 0) {
		return default_recv_base_minimum_delay_ms_;
	}

	auto stream = receive_streams_.find(ssrc);
	if (stream != receive_streams_.end()) {
		return absl::nullopt;	
	}
	else {
		RTC_LOG(LS_ERROR) << "No stream found to get base minimum playout delay";
		return absl::nullopt;
	}
}

bool WebRtcVideoChannel::SendRtp(const uint8_t* data,
	size_t len,
	const webrtc::PacketOptions& options) {
	rtc::CopyOnWriteBuffer packet(data, len, kMaxRtpPacketLen);
	rtc::PacketOptions rtc_options;
	rtc_options.packet_id = options.packet_id;
	if (DscpEnabled()) {
		rtc_options.dscp = PreferredDscp();
	}
	rtc_options.info_signaled_after_sent.included_in_feedback =
		options.included_in_feedback;
	rtc_options.info_signaled_after_sent.included_in_allocation =
		options.included_in_allocation;
	return MediaChannel::SendPacket(&packet, rtc_options);
}

bool WebRtcVideoChannel::SendRtcp(const uint8_t* data, size_t len) {
	rtc::CopyOnWriteBuffer packet(data, len, kMaxRtpPacketLen);
	rtc::PacketOptions rtc_options;
	if (DscpEnabled()) {
		rtc_options.dscp = PreferredDscp();
	}

	return MediaChannel::SendRtcp(&packet, rtc_options);
}

WebRtcVideoChannel::VideoCodecSettings::VideoCodecSettings()
	: flexfec_payload_type(-1), rtx_payload_type(-1) {}

bool WebRtcVideoChannel::VideoCodecSettings::operator==(
	const WebRtcVideoChannel::VideoCodecSettings& other) const {
	return codec == other.codec && ulpfec == other.ulpfec &&
		flexfec_payload_type == other.flexfec_payload_type &&
		rtx_payload_type == other.rtx_payload_type;
}

bool WebRtcVideoChannel::VideoCodecSettings::EqualsDisregardingFlexfec(
	const WebRtcVideoChannel::VideoCodecSettings& a,
	const WebRtcVideoChannel::VideoCodecSettings& b) {
	return a.codec == b.codec && a.ulpfec == b.ulpfec &&
		a.rtx_payload_type == b.rtx_payload_type;
}

bool WebRtcVideoChannel::VideoCodecSettings::operator!=(
	const WebRtcVideoChannel::VideoCodecSettings& other) const {
	return !(*this == other);
}


std::vector<WebRtcVideoChannel::VideoCodecSettings>
WebRtcVideoChannel::MapCodecs(const std::vector<VideoCodec>& codecs) {
	if (codecs.empty()) {
		return{};
	}

	std::vector<VideoCodecSettings> video_codecs;
	std::map<int, VideoCodec::CodecType> payload_codec_type;
	// |rtx_mapping| maps video payload type to rtx payload type.
	std::map<int, int> rtx_mapping;

	webrtc::UlpfecConfig ulpfec_config;
	absl::optional<int> flexfec_payload_type;

	for (const VideoCodec& in_codec : codecs) {
		const int payload_type = in_codec.id;

		if (payload_codec_type.find(payload_type) != payload_codec_type.end()) {
			RTC_LOG(LS_ERROR) << "Payload type already registered: "
				<< in_codec.ToString();
			return{};
		}
		payload_codec_type[payload_type] = in_codec.GetCodecType();

		switch (in_codec.GetCodecType()) {
		case VideoCodec::CODEC_RED: {
			if (ulpfec_config.red_payload_type != -1) {
				RTC_LOG(LS_ERROR)
					<< "Duplicate RED codec: ignoring PT=" << payload_type
					<< " in favor of PT=" << ulpfec_config.red_payload_type
					<< " which was specified first.";
				break;
			}
			ulpfec_config.red_payload_type = payload_type;
			break;
		}

		case VideoCodec::CODEC_ULPFEC: {
			if (ulpfec_config.ulpfec_payload_type != -1) {
				RTC_LOG(LS_ERROR)
					<< "Duplicate ULPFEC codec: ignoring PT=" << payload_type
					<< " in favor of PT=" << ulpfec_config.ulpfec_payload_type
					<< " which was specified first.";
				break;
			}
			ulpfec_config.ulpfec_payload_type = payload_type;
			break;
		}

		case VideoCodec::CODEC_FLEXFEC: {
			if (flexfec_payload_type) {
				RTC_LOG(LS_ERROR)
					<< "Duplicate FLEXFEC codec: ignoring PT=" << payload_type
					<< " in favor of PT=" << *flexfec_payload_type
					<< " which was specified first.";
				break;
			}
			flexfec_payload_type = payload_type;
			break;
		}

		case VideoCodec::CODEC_RTX: {
			int associated_payload_type;
			if (!in_codec.GetParam(kCodecParamAssociatedPayloadType,
				&associated_payload_type) ||
				!IsValidRtpPayloadType(associated_payload_type)) {
				RTC_LOG(LS_ERROR)
					<< "RTX codec with invalid or no associated payload type: "
					<< in_codec.ToString();
				return{};
			}
			rtx_mapping[associated_payload_type] = payload_type;
			break;
		}

		case VideoCodec::CODEC_VIDEO: {
			video_codecs.emplace_back();
			video_codecs.back().codec = in_codec;
			break;
		}
		}
	}

	// One of these codecs should have been a video codec. Only having FEC
	// parameters into this code is a logic error.
	RTC_DCHECK(!video_codecs.empty());

	for (const auto& entry : rtx_mapping) {
		const int associated_payload_type = entry.first;
		const int rtx_payload_type = entry.second;
		auto it = payload_codec_type.find(associated_payload_type);
		if (it == payload_codec_type.end()) {
			RTC_LOG(LS_ERROR) << "RTX codec (PT=" << rtx_payload_type
				<< ") mapped to PT=" << associated_payload_type
				<< " which is not in the codec list.";
			return{};
		}
		const VideoCodec::CodecType associated_codec_type = it->second;
		if (associated_codec_type != VideoCodec::CODEC_VIDEO &&
			associated_codec_type != VideoCodec::CODEC_RED) {
			RTC_LOG(LS_ERROR)
				<< "RTX PT=" << rtx_payload_type
				<< " not mapped to regular video codec or RED codec (PT="
				<< associated_payload_type << ").";
			return{};
		}

		if (associated_payload_type == ulpfec_config.red_payload_type) {
			ulpfec_config.red_rtx_payload_type = rtx_payload_type;
		}
	}

	for (VideoCodecSettings& codec_settings : video_codecs) {
		const int payload_type = codec_settings.codec.id;
		codec_settings.ulpfec = ulpfec_config;
		codec_settings.flexfec_payload_type = flexfec_payload_type.value_or(-1);
		auto it = rtx_mapping.find(payload_type);
		if (it != rtx_mapping.end()) {
			const int rtx_payload_type = it->second;
			codec_settings.rtx_payload_type = rtx_payload_type;
		}
	}

	return video_codecs;
}

// TODO WebRtcVideoSendStream, WebRtcVideoRecvStream

}