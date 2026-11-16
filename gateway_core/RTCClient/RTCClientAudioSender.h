#ifndef __RTC_CLIENT_AUDIO_SENDER_H__
#define __RTC_CLIENT_AUDIO_SENDER_H__

#include <mutex>

#include "api/scoped_refptr.h"

#include "../RTCPeerConnection/rtc_pc_src/api/RTCRtpTransceiverInterface.h"
#include "../RTCPeerConnection/rtc_pc_src/api/audio/RTCAudioPacketRouteSourceInterface.h"

#include "RTCAudioSenderTickInterface.h"
#include "RTCAudioOuterSetInterface.h"
#include "../RTCUtils/RTCSectionProfiler.h"

namespace rtc {
class RTCAudioRouteTrackSource;
}

namespace webrtc {
class RTCPeer;
class RTCAudioRouteTrack;
}

class RTCClientInterface;

class RTCClientAudioSender : public RTCAudioSenderTickInterface,
                             public RTCAudioEgressSourceSetInterface {
public:
    explicit RTCClientAudioSender(uint32_t ssrc, RTCClientInterface* peer);
    ~RTCClientAudioSender();
    
public:
	static constexpr uint8_t kOpusSilence[3] = {0xFC, 0xFF, 0xFE};	    // F8 mono
    static constexpr int     kOpusClockKHz   = 48;                 // 48kHz
    static constexpr int64_t kMarkerGapUs    = 40000;  
    static constexpr uint32_t kOpusFrameDurationMs = 20;
    static constexpr uint32_t kOpusTimestampIncrement = kOpusClockKHz * kOpusFrameDurationMs;

public:    
    status_t CreateTrack(bool is_unified_plan = true);
    void RemoveTrack();
    bool CheckSender();

    // RTCAudioSenderTickInterface method
    void OnTick() override;
    
    // RTCAudioEgressSourceSetInterface method                                
    void SetEgressAudioSource(RTCEgressAudioSourceInterface *outerSource) override;    

private:
    void _on_send_packet();

private:
    uint32_t _ssrc;
    RTCClientInterface* _peer;
    std::string _mid;
	uint16_t _seq = 0;
    uint8_t _pt = 0xFF;
    int32_t _opus_sample_rate;
    int32_t _opus_channels;

    uint32_t _ts_base     = 0;      
    int64_t  _t0_us       = 0;      
    int64_t  _last_send_us = 0;     
    bool     _marker_pending = true;  
    uint32_t _packets_sent = 0;     
    uint32_t _octets_sent  = 0;   
    bool     _has_encoding_frame = false;
    uint32_t _next_ts = 0;   

	rtc::scoped_refptr<webrtc::RTCRtpTransceiverInterface> _transceiver;
    rtc::scoped_refptr<rtc::RTCAudioRouteTrackSource> _route_source;
    rtc::scoped_refptr<webrtc::RTCAudioRouteTrack> _route_track;
	rtc::scoped_refptr<webrtc::RTCRtpSenderInterface> _route_sender; 
    
    std::mutex _outer_source_lock;
    RTCEgressAudioSourceInterface* _outer_source;
    uint8_t _enc_buffer[2048];   // TODO const
     
    /////////////////////////////////////
    // profile
#ifdef __PERFORMANCE_PROFILE_ENABLED__    
    RTCSectionStat _forward_packet{"forward_packet"};
#endif

    // for debug
    uint32_t _last_ts;

private:
    RTCClientAudioSender(const RTCClientAudioSender&) = delete;
    RTCClientAudioSender& operator=(const RTCClientAudioSender&) = delete;     
};

#endif // __RTC_CLIENT_AUDIO_SENDER_H__