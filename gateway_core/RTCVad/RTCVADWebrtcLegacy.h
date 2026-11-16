#ifndef __RTC_VAD_WEBRTC_LEGACY_H__
#define __RTC_VAD_WEBRTC_LEGACY_H__

#include "common_audio/vad/include/vad.h"
#include "RTCVADInterface.h"

class RTCVADWebrtcLegacy : public RTCVADInterface {
public:
    RTCVADWebrtcLegacy() = default;
    virtual ~RTCVADWebrtcLegacy() = default;

public:
    status_t SetConfig(int32_t sampleRate, int32_t channels, VADAggressiveness mode) override;
    VADActivity VoiceActivity(int16_t* pcm, int32_t samples) override;  
    
private:
    int32_t _sample_rate;
    int32_t _channels;
    VADAggressiveness _mode;
    std::unique_ptr<webrtc::Vad> _vad; 
    
private:
    RTCVADWebrtcLegacy(const RTCVADWebrtcLegacy&) = delete;
    RTCVADWebrtcLegacy& operator=(const RTCVADWebrtcLegacy&) = delete;    
};

#endif // __RTC_VAD_WEBRTC_LEGACY_H__