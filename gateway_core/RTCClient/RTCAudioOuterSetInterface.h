#ifndef __RTC_AUDIO_OUTER_SET_INTERFACE_H__
#define __RTC_AUDIO_OUTER_SET_INTERFACE_H__

#include "RTCIngressAudioPacketSinkInterface.h"
#include "RTCEgressAudioSourceInterface.h"

class RTCAudioIngressSinkSetInterface {
public:
    RTCAudioIngressSinkSetInterface() = default;
    virtual ~RTCAudioIngressSinkSetInterface() = default;   

public:
    virtual void setIngressSink(RTCIngressAudioPacketSinkInterface* ingressSink) = 0;    
};

class RTCAudioEgressSourceSetInterface {
public:
    RTCAudioEgressSourceSetInterface() = default;
    virtual ~RTCAudioEgressSourceSetInterface() = default; 

public:
   virtual void SetEgressAudioSource(RTCEgressAudioSourceInterface *outerSource) = 0;
};

#endif // __RTC_AUDIO_OUTER_INTERFACE_H__