#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif

#define LOG_TAG "RTCClientAudioSender"
#include <utils/Log.h>
#include <utils/Errors.h>

#include <cassert>
#include <string>
#include <cstring>            // memcpy
#include <random>             // std::random_device

#include "api/rtp_parameters.h"
#include "api/rtp_headers.h"
#include "modules/rtp_rtcp/source/rtp_packet_received.h"
#include "rtc_base/time_utils.h" 
#include "absl/strings/match.h"

#include "../RTCPeerConnection/rtc_pc_src/_deprecate_defines.h"
#include "../RTCPeerConnection/rtc_pc_src/RTCFunctionTracer.h"
#include "../RTCPeerConnection/rtc_pc_src/api/RTCPeer.h"

#include "../RTCPeerConnection/rtc_pc_src/media/base/RTCAudioRouteTrackSource.h"
#include "../RTCPeerConnection/rtc_pc_src/pc/RTCAudioRouteTrack.h"
#include "../RTCAudioFormat/RTCAudioFormat.h"

#include "RTCClientInterface.h"
#include "RTCClientAudioSender.h"


RTCClientAudioSender::RTCClientAudioSender(uint32_t ssrc, 
    RTCClientInterface* peer) 
  : _ssrc(ssrc), 
    _peer(peer) {
 //   RTC_CLASS_FUNCTION_TRACER("RTCClientAudioSender::RTCClientAudioSender", this);           
    std::random_device rd;
    _ts_base = rd();
    _t0_us = rtc::TimeMicros();
    _last_send_us = _t0_us;
    _next_ts = _ts_base;
}

RTCClientAudioSender::~RTCClientAudioSender() {

}

status_t RTCClientAudioSender::CreateTrack(bool is_unified_plan /*= true*/) {
    _route_source = _peer->PeerPtr()->CreateAudioRouteTrackSource();
	if (_route_source == nullptr) {
		return INVALID_OPERATION;
	}

	std::string track_id = std::to_string((unsigned)_ssrc);
	_route_track = _peer->PeerPtr()->CreateAudioRouteTrack(track_id, _route_source);
	if (_route_track == nullptr) {
		return INVALID_OPERATION;
	}

	_route_track->set_enabled(true);

	std::vector<std::string> stream_labels;
	stream_labels.push_back(track_id);
    
    webrtc::RTCRtpTransceiverInit init;
    init.direction = webrtc::RtpTransceiverDirection::kSendRecv;
    init.stream_ids = stream_labels;

    _transceiver = _peer->PeerPtr()->AddTransceiver(_route_track, init);
    if (_transceiver != nullptr) {
        _route_sender = _transceiver->sender();
        _route_sender->SetOutSsrc(_ssrc);
    }
    else {
        return INVALID_OPERATION;
    }
   
	return OK;    
}

bool RTCClientAudioSender::CheckSender() {
 //   RTC_CLASS_FUNCTION_TRACER("RTCClientAudioSender::CheckSender", this);      
    absl::optional<std::string> mid = _transceiver->mid();
    if (mid == absl::nullopt) {
        ALOGW("[%x] RTCVideoConsumer::is_check_transceiver() mid nothing", this);
        return false;
    }
    _mid = mid.value();

    webrtc::RtpParameters params = _route_sender->GetParameters();
    
    for (const auto& c : params.codecs) {
        ALOGI("codec name=%s pt=%d", c.name.c_str(), c.payload_type);
        if (absl::EqualsIgnoreCase(c.name, "opus")) {
            _pt = (uint8_t)c.payload_type;

            _opus_sample_rate = c.clock_rate.value();
            _opus_channels = c.num_channels.value();

            break;
        }
    }

    return true;
}

void RTCClientAudioSender::RemoveTrack() {
	if (_route_sender == nullptr) {
		return;
	}

	_transceiver->StopInternal();

	_peer->PeerPtr()->RemoveTrack(_route_sender);
	_route_sender = nullptr;
}  

void RTCClientAudioSender::SetEgressAudioSource(RTCEgressAudioSourceInterface *outerSource) {
    std::lock_guard<std::mutex> lock(_outer_source_lock);
    _outer_source = outerSource;

    if(_outer_source != nullptr) {
        RTCAudioFormat format;
        format.codec = RTCAudioFormat::Codec::kOpus;
        format.sample_rate = _opus_sample_rate;
        format.channels = _opus_channels;
        _outer_source->SetEgressFormat(format);        
    }
}

void RTCClientAudioSender::OnTick() {
    if(_pt == 0xFF) {
        return;
    }    
    _on_send_packet();
}

void RTCClientAudioSender::_on_send_packet() {
    const int64_t now_us = rtc::TimeMicros();    
    const uint32_t  ts = _next_ts;
    
    std::unique_ptr<webrtc::RtpPacketReceived> routing_packet = std::make_unique<webrtc::RtpPacketReceived>();
    routing_packet->SetPayloadType(_pt);       
    routing_packet->SetSsrc(_ssrc);


    uint8_t* payload;

    int64_t bfus, afus;

    size_t outsize = sizeof(kOpusSilence);
    {
        std::lock_guard<std::mutex> lock(_outer_source_lock);
        bfus = rtc::TimeMicros();
        if(_outer_source == nullptr || !_outer_source->PullEncodedFrame(_enc_buffer, sizeof(_enc_buffer), &outsize)) {
            payload = routing_packet->AllocatePayload(sizeof(kOpusSilence));
            memcpy(payload, kOpusSilence, sizeof(kOpusSilence));
            _has_encoding_frame = false;   
        } else {
            payload = routing_packet->AllocatePayload(outsize);
            memcpy(payload, _enc_buffer, outsize);
            if(_has_encoding_frame == false) {
                _has_encoding_frame = true;
                _marker_pending = true;
            } 
        }
        afus = rtc::TimeMicros();
    }

    routing_packet->SetTimestamp(ts);

    const bool marker = _marker_pending ||
                        (now_us - _last_send_us > kMarkerGapUs);  

    routing_packet->SetMarker(marker);                           

    routing_packet->set_arrival_time_ms(now_us/1000);
    routing_packet->set_first_packet_in_frame(); 
    
    routing_packet->SetSequenceNumber(static_cast<uint16_t>(_seq));
    _seq++;
    _next_ts += kOpusTimestampIncrement;
    _last_send_us   = now_us;
    _last_ts = ts;
    _marker_pending = false;
    _packets_sent++;
    _octets_sent   += outsize;

    {
#ifdef __PERFORMANCE_PROFILE_ENABLED__        
        RTCScopedSectionTimer forward_timer(_forward_packet);
#endif        
        _route_source->OnRtpPacket(std::move(routing_packet));
    }

#ifdef __PERFORMANCE_PROFILE_ENABLED__    
    if (_forward_packet.ReportDue()) {
         ALOGI("profile_trace %s", _forward_packet.TakeReport().c_str());
    }
#endif    
}

