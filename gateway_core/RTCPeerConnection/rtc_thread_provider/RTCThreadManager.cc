//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#include "rtc_base/logging.h"
#include "rtc_base/checks.h"

#include "../rtc_pc_src/RTCFunctionTracer.h"

#include "RTCThreadManager.h"

namespace rtc {

RTCThreadManager::RTCThreadManager() 
 : _thread_manager(nullptr),
   _msg_handler(nullptr),
   _network_thread(nullptr),
   _worker_thread(nullptr),
   _signaling_thread(nullptr) {

 //   RTC_CLASS_FUNCTION_TRACER("RTCThreadManager::RTCThreadManager", this);
    _thread_manager = std::unique_ptr<rtc::ThreadManager>(new rtc::ThreadManager((void*)this));
    _msg_handler = std::unique_ptr<rtc::RTCAnyInvocableMessageHandler>(new rtc::RTCAnyInvocableMessageHandler);

    _create_threads();    
}

RTCThreadManager::~RTCThreadManager() {
//    RTC_CLASS_FUNCTION_TRACER("RTCThreadManager::~RTCThreadManager", this);
    // thinking....
    _network_thread.reset();
    _worker_thread.reset();
    _signaling_thread.reset();
    _msg_handler.reset();
    _thread_manager.reset();
}

rtc::Thread* RTCThreadManager::signaling_thread() {
    return _signaling_thread.get();
}

rtc::Thread* RTCThreadManager::worker_thread() {
    return _worker_thread.get();
}

rtc::Thread* RTCThreadManager::network_thread() {
    return _network_thread.get();
}

// RTCThreadManagerInterface
rtc::ThreadManager* RTCThreadManager::Instance() {
    return _thread_manager.get();
}

void RTCThreadManager::Add(rtc::Thread* message_queue) {
    if (_thread_manager) {
        _thread_manager->AddInternal(message_queue);
    }
}

void RTCThreadManager::Remove(rtc::Thread* message_queue) {
    if (_thread_manager) {
        _thread_manager->RemoveInternal(message_queue);
    }
}

void RTCThreadManager::Clear(rtc::MessageHandler* handler) {
    if (_thread_manager) {
        _thread_manager->ClearInternal(handler);
    }
}

void* RTCThreadManager::GetAnyInvocableMessageHandler() {
    if (_msg_handler) {
        return (void*)_msg_handler.get();
    }
    return nullptr;
}

void RTCThreadManager::_create_threads() {
//    RTC_CLASS_FUNCTION_TRACER("RTCThreadManager::_create_threads", this);
    _network_thread = rtc::Thread::CreateWithSocketServer((void*)((rtc::RTCThreadManagerInterface*)this));
    _network_thread->SetName("network_thread", nullptr);
    RTC_CHECK(_network_thread->Start()) << "Failed to start thread";

    _worker_thread = rtc::Thread::Create((void*)((rtc::RTCThreadManagerInterface*)this));
    _worker_thread->SetName("worker_thread", nullptr);
    RTC_CHECK(_worker_thread->Start()) << "Failed to start thread";

    _signaling_thread = rtc::Thread::Create((void*)((rtc::RTCThreadManagerInterface*)this));
    _signaling_thread->SetName("signaling_thread", nullptr);
    RTC_CHECK(_signaling_thread->Start()) << "Failed to start thread";

//    RTC_LOG(LS_INFO) << "kimi thread_crash_trace RTCThreadManager::_create_threads this "<< this << " _network_thread "<< _network_thread.get() << " _worker_thread "<< _worker_thread.get() << " _signaling_thread "<< _signaling_thread.get();
}

}
