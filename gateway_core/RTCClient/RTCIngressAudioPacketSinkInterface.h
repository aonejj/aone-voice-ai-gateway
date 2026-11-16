#ifndef __RTC_INGRESS_AUDIO_PACKET_SINK_INTERFACE_H__
#define __RTC_INGRESS_AUDIO_PACKET_SINK_INTERFACE_H__

#include "../RTCAudioFormat/RTCAudioFormat.h"

struct RTCIngressAudioFrame {
    const uint8_t* data;  
    size_t size;
    uint32_t rtp_timestamp;
    uint16_t sequence_number;
    int64_t receive_time_ms;  
};


class RTCIngressAudioPacketSinkInterface {
public:
    RTCIngressAudioPacketSinkInterface() = default;
    virtual ~RTCIngressAudioPacketSinkInterface() = default;

public:
    virtual void SetIngressFormat(const RTCAudioFormat& format) = 0;
    virtual void OnAudioFrame(const RTCIngressAudioFrame& frame) = 0;
};


#endif // __RTC_INGRESS_AUDIO_PACKET_SINK_INTERFACE_H__