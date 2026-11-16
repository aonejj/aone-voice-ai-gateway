#ifndef __RTC_AUDIO_TICK_SCHEDULER_H__
#define __RTC_AUDIO_TICK_SCHEDULER_H__

#include <utils/Errors.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "../RTCClient/RTCAudioSenderSchedulerInterface.h"

class RTCAudioTickScheduler : public RTCAudioSenderSchedulerInterface {
public:
    static constexpr int64_t kTickIntervalUs = 20000;   // 20ms
    static constexpr int64_t kResyncThresholdUs = 20000; // 1 tick 

    struct Stats {
        uint64_t ticks = 0;
        uint64_t late_ticks = 0;      
        int64_t  max_tick_cost_us = 0;
        int64_t  max_lateness_us = 0; 
        size_t   sender_count = 0;
    };

    RTCAudioTickScheduler();
    ~RTCAudioTickScheduler();

    bool Start(int worker_count = 1);
    void Stop();
    Stats GetStats();

    bool Register(RTCAudioSenderTickInterface* sender) override;
    void Unregister(RTCAudioSenderTickInterface* sender) override;

private:
    struct Worker {
        std::thread thread;
        std::mutex mutex;                      
        std::vector<RTCAudioSenderTickInterface*> senders;
        std::atomic<bool> running{false};

        std::atomic<uint64_t> ticks{0};
        std::atomic<uint64_t> late_ticks{0};
        std::atomic<int64_t>  max_tick_cost_us{0};
        std::atomic<int64_t>  max_lateness_us{0};
    };

    void _worker_loop(Worker* w);

    static int64_t _now_us();                   // CLOCK_MONOTONIC
    static void _sleep_until_us(int64_t abs_us);

    std::mutex _state_mutex;                   
    std::vector<std::unique_ptr<Worker>> _workers;
    bool _started = false;

private:
    RTCAudioTickScheduler(const RTCAudioTickScheduler&) = delete;
    RTCAudioTickScheduler& operator=(const RTCAudioTickScheduler&) = delete;   
};

#endif  // __RTC_AUDIO_TICK_SCHEDULER_H__