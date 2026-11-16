//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#ifndef __RTC_THREAD_MANAGER_POOL_H__
#define __RTC_THREAD_MANAGER_POOL_H__  

#include <vector>
#include <memory>
#include <atomic>

namespace rtc {

class RTCThreadManager;    

class RTCThreadManagerPool {
public:
    static RTCThreadManagerPool& Instance();
    RTCThreadManager* GetThreadManager();


private:
    RTCThreadManagerPool();
    ~RTCThreadManagerPool();

    RTCThreadManagerPool(const RTCThreadManagerPool&) = delete;
    RTCThreadManagerPool& operator=(const RTCThreadManagerPool&) = delete;

private:
    std::vector<std::unique_ptr<RTCThreadManager>> managers_;
    std::atomic<size_t> next_manager_{0};    
};

}

#endif // __RTC_THREAD_MANAGER_POOL_H__ 