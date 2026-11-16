#ifndef __RTC_AUDIO_SENDER_TICK_INTERFACE_H__
#define __RTC_AUDIO_SENDER_TICK_INTERFACE_H__

class RTCAudioSenderTickInterface;

class RTCAudioSenderTickInterface {
public:
    RTCAudioSenderTickInterface() = default;
    ~RTCAudioSenderTickInterface() = default;

public:
    virtual void OnTick() = 0;

private:
    RTCAudioSenderTickInterface(const RTCAudioSenderTickInterface&) = delete;
    RTCAudioSenderTickInterface& operator=(const RTCAudioSenderTickInterface&) = delete;    
};    


#endif // __RTC_AUDIO_SENDER_TICK_INTERFACE_H__