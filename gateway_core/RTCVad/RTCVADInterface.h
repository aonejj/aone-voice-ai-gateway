#ifndef __RTC_VAD_INTERFACE_H__
#define __RTC_VAD_INTERFACE_H__

#include <utils/Errors.h>

class RTCVADInterface {
public:
    RTCVADInterface() = default;
    virtual ~RTCVADInterface() = default;

public:
    enum VADAggressiveness {
        kVadNormal = 0,
        kVadLowBitrate = 1,
        kVadAggressive = 2,
        kVadVeryAggressive = 3
    };

    enum VADActivity { 
        kPassive = 0, 
        kActive = 1, 
        kError = -1 
    };  

public:
    virtual status_t SetConfig(int32_t sampleRate, int32_t channls, VADAggressiveness mode) = 0;
    virtual VADActivity VoiceActivity(int16_t* pcm, int32_t samples) = 0;

private:
    RTCVADInterface(const RTCVADInterface&) = delete;
    RTCVADInterface& operator=(const RTCVADInterface&) = delete;     
};

#endif // __RTC_VAD_INTERFACE_H__