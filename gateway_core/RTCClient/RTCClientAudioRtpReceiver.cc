//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif

#define LOG_TAG "RTCClientAudioRtpReceiver"
#include <string>
#include <cassert>

#include <utils/Log.h>
#include <utils/Errors.h>

#include "modules/rtp_rtcp/source/rtp_header_extensions.h"
#include "rtc_base/time_utils.h"

#include "../RTCPeerConnection/rtc_pc_src/_deprecate_defines.h"
#include "../RTCPeerConnection/rtc_pc_src/RTCFunctionTracer.h"
#include "../RTCCodec/RTCCodecFactory.h"

#include "RTCClientInterface.h"
#include "RTCClientAudioRtpReceiver.h"


RTCClientAudioRtpReceiver::RTCClientAudioRtpReceiver(rtc::scoped_refptr<webrtc::RTCRtpTransceiverInterface> transceiver,
												 RTCClientInterface* client) 
	: _transceiver(transceiver),
	  _client(client) {

}

RTCClientAudioRtpReceiver::~RTCClientAudioRtpReceiver() {

}

std::vector<webrtc::RtpParameters> RTCClientAudioRtpReceiver::getRtpParameters() {
	if(!_is_set_parameter) {
		_rtp_parameters = _transceiver->receiver()->GetParameters();
		_construct_rtp_parameters(_rtp_parameters);
//		_dump_transceiver(_transceiver);
		_transceiver = nullptr;
		_is_set_parameter = true;
	}

	std::vector<webrtc::RtpParameters> vec;
	vec.push_back(_rtp_parameters);
	return vec;
}

absl::optional<webrtc::RtpHeaderExtensionMap> RTCClientAudioRtpReceiver::getRtpHeaderExtensionMap() {
	return _rtp_extensions;
}

const RTCClientInterface* RTCClientAudioRtpReceiver::getRTCClientInterface() const {
	return _client;
}

void RTCClientAudioRtpReceiver::setIngressSink(RTCIngressAudioPacketSinkInterface* ingressSink) {
	std::lock_guard<std::mutex> lock(_sink_lock);
	_ingress_sink = ingressSink;
}

void RTCClientAudioRtpReceiver::close() {
	if(_audio_decoder != nullptr) {
		_audio_decoder.reset();
	}
}

void RTCClientAudioRtpReceiver::OnRtpPacket(const webrtc::RtpPacketReceived& packet) {
#ifdef __ENABLE_DEBUGING_PACKET_PATH_TRACE__
  PACKET_TRACE_INFO dinfo;
  dinfo._trace_info = "peer_audio_rtp_packet_recv";
  dinfo._time_us = rtc::TimeMicros();
  packet._debug_packet_trace_vector.push_back(dinfo);
//  _dump_packet_trace_info("until_call_deliver_rtp", packet._debug_packet_trace_vector);
#endif 

	if (_init_pt == -1) {
		_init_pt = packet.PayloadType();
		auto it = _codec_parameters.find(_init_pt);
		if (it != _codec_parameters.end()) {
			if (it->second.name == "opus") {
				RTCAudioFormat format;
				format.codec = RTCAudioFormat::Codec::kOpus;
				format.sample_rate = it->second.sampling_rate;
				format.channels = it->second.num_channels;

				{
					std::lock_guard<std::mutex> lock(_sink_lock);
					if(_ingress_sink != nullptr) {
						_ingress_sink->SetIngressFormat(format);
					}
				}
			}
		} else {
			assert(0);
		}
	}

	if(packet.payload_size() == 0) {
		return;
	}

	RTCIngressAudioFrame ingressFrame;
	ingressFrame.data = (uint8_t*)packet.PayloadBuffer().cdata();
	ingressFrame.size = packet.payload_size();
	ingressFrame.rtp_timestamp = packet.Timestamp();
	ingressFrame.sequence_number = packet.SequenceNumber();
	ingressFrame.receive_time_ms = packet.arrival_time_ms();

	{
		std::lock_guard<std::mutex> lock(_sink_lock);
		if(_ingress_sink != nullptr) {
			_ingress_sink->OnAudioFrame(ingressFrame);
		}
	}
}

void RTCClientAudioRtpReceiver::SetRtpHeaderExtensionMap(absl::optional<webrtc::RtpHeaderExtensionMap> extMap) {
	_rtp_extensions = extMap;
}

void RTCClientAudioRtpReceiver::_construct_rtp_parameters(webrtc::RtpParameters& parameters) {
	ALOGI("_construct_rtp_parameters");
	for (auto it : parameters.codecs) {
		RTCRtpCodecParameter tmp;
		tmp.mime_type = it.mime_type();
		tmp.name = it.name;
		if (it.clock_rate != absl::nullopt) {
			tmp.sampling_rate = it.clock_rate.value();
		}
		if (it.num_channels != absl::nullopt) {
			tmp.num_channels = it.num_channels.value();
		}

		int32_t pt = it.payload_type;
		ALOGI("pt  %d name %s", pt, tmp.name.c_str());
		_codec_parameters[pt] = tmp;
	}
}

void RTCClientAudioRtpReceiver::_dump_transceiver(rtc::scoped_refptr<webrtc::RTCRtpTransceiverInterface> transceiver) {
	ALOGV("[%x] _dump_transceiver +++++++++++++++++++++", this);
	cricket::MediaType media_type = transceiver->media_type();
	ALOGD("media_type  %s", media_type == cricket::MediaType::MEDIA_TYPE_AUDIO ? "audio" : "video");
	absl::optional<std::string> mid = transceiver->mid();
	if (mid != absl::nullopt) {
		ALOGD("mid %s", mid.value().c_str());
	}
	
	rtc::scoped_refptr<webrtc::RTCRtpReceiverInterface> receiver = transceiver->receiver();
	uint32_t ssrc = receiver->ssrc();
	ALOGD("ssrc %u", ssrc);
	webrtc::RtpParameters rtpParameters = receiver->GetParameters();

	ALOGD("******* RtpCodecParameters *******");
	for (auto it : rtpParameters.codecs) {
		ALOGD("mime_type %s", it.mime_type().c_str());
		ALOGD("name %s", it.name.c_str());
		ALOGD("payload_type %d", it.payload_type);
		if (it.clock_rate != absl::nullopt) {
			ALOGD("clock rate %d", it.clock_rate.value());
		}
		if (it.num_channels != absl::nullopt) {
			ALOGD("num_channels %d", it.num_channels.value());
		}
	}

	ALOGD("******* RtpExtension *******");
	for (auto it : rtpParameters.header_extensions) {
		std::string tmp = it.ToString();
		ALOGD("%s", tmp.c_str());
	}

	ALOGD("******* RtpEncodingParameters *******");
	for (auto it : rtpParameters.encodings) {
		if (it.ssrc != absl::nullopt) {
			ALOGD("ssrc %u", it.ssrc.value());
		}
	}

	ALOGV("[%x] _dump_transceiver ---------------------", this);
}