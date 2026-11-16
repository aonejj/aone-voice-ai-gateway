//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_THREAD_MANAGER_H__
#define __RTC_THREAD_MANAGER_H__

#include <memory>

#include "rtc_base/thread.h"
#include "rtc_base/message_handler.h"
#include "rtc_base/thread_message.h"

#include "../rtc_pc_src/RTCThreadManagerInterface.h"
#include "../rtc_pc_src/RTCInvocableMessageHandler.h"

namespace rtc {

class RTCThreadManager : public RTCThreadManagerInterface {
public:
    RTCThreadManager();
    ~RTCThreadManager();

public:
	rtc::Thread* signaling_thread();
	rtc::Thread* worker_thread();
	rtc::Thread* network_thread();

public:
    // RTCThreadManagerInterface
    rtc::ThreadManager* Instance() override;
    void Add(rtc::Thread* message_queue) override;
    void Remove(rtc::Thread* message_queue) override;
    void Clear(rtc::MessageHandler* handler) override;
    void* GetAnyInvocableMessageHandler() override;

private:
    void _create_threads();    

private:
	std::unique_ptr<rtc::ThreadManager> _thread_manager;
	std::unique_ptr<rtc::RTCAnyInvocableMessageHandler> _msg_handler;    

	std::unique_ptr<rtc::Thread> _network_thread;
	std::unique_ptr<rtc::Thread> _worker_thread;
	std::unique_ptr<rtc::Thread> _signaling_thread;    
};

}

#endif // __RTC_THREAD_MANAGER_H__