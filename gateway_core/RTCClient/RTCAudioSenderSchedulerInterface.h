#ifndef __RTC_AUDIO_SENDER_SCHEDULER_INTERFACE_H__
#define __RTC_AUDIO_SENDER_SCHEDULER_INTERFACE_H__


class RTCAudioSenderTickInterface;

class RTCAudioSenderSchedulerInterface {
public:
    RTCAudioSenderSchedulerInterface() = default;
    ~RTCAudioSenderSchedulerInterface() = default;

public:
    virtual bool Register(RTCAudioSenderTickInterface* sender) = 0;
    virtual void Unregister(RTCAudioSenderTickInterface* sender) = 0;

private:
    RTCAudioSenderSchedulerInterface(const RTCAudioSenderSchedulerInterface&) = delete;
    RTCAudioSenderSchedulerInterface& operator=(const RTCAudioSenderSchedulerInterface&) = delete;    
};    


#endif // __RTC_AUDIO_SENDER_SCHEDULER_INTERFACE_H__