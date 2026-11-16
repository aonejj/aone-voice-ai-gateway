//////////////////////////////////////////////////////////////////////////
//
// author : kimi
//
//////////////////////////////////////////////////////////////////////////

#include "rtc_base/logging.h"
#include "rtc_base/checks.h"

#include "RTCThreadManager.h"
#include "RTCThreadManagerPool.h"

#include "../rtc_pc_src/RTCFunctionTracer.h"

const size_t kThreadManagers = 2; // cpu count

namespace rtc {

RTCThreadManagerPool& RTCThreadManagerPool::Instance() {
    static RTCThreadManagerPool instance;
    return instance;
}

RTCThreadManager* RTCThreadManagerPool::GetThreadManager() {
    size_t index = next_manager_.fetch_add(1) % managers_.size();
 //   RTC_CLASS_FUNCTION_TRACER("RTCThreadManagerPool::GetThreadManager", managers_[index].get());
    return managers_[index].get();
}


RTCThreadManagerPool::RTCThreadManagerPool() {
    // Create a fixed number of RTCThreadManager instances
    for (size_t i = 0; i < kThreadManagers; ++i) {
        managers_.emplace_back(std::make_unique<RTCThreadManager>());
    }
}

RTCThreadManagerPool::~RTCThreadManagerPool() {
    // Clean up the RTCThreadManager instances
    managers_.clear();
}

}