#ifndef __RTC_TIME_UTILS_H__
#define __RTC_TIME_UTILS_H__

#include <chrono>

class RTCTimeUtils {
public:
    using Clock = std::chrono::steady_clock;

    static int64_t now_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now().time_since_epoch()).count();
    }

    static int64_t now_us() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now().time_since_epoch()).count();
    }    

    static int64_t now_ns() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            Clock::now().time_since_epoch()).count();
    }    
};

#endif // __RTC_TIME_UTILS_H__
