#ifndef __RTC_EGRESS_AUDIO_SOURCE_INTERFACE_H__
#define __RTC_EGRESS_AUDIO_SOURCE_INTERFACE_H__

#include "../RTCAudioFormat/RTCAudioFormat.h"

class RTCEgressAudioSourceInterface {
public:
    RTCEgressAudioSourceInterface() = default;
    virtual ~RTCEgressAudioSourceInterface() = default;

public:    
    virtual void SetEgressFormat(const RTCAudioFormat& format) = 0;
    virtual bool PullEncodedFrame(uint8_t* buf, size_t capacity, size_t* size) = 0;
};

#endif // __RTC_EGRESS_AUDIO_SOURCE_INTERFACE_H__