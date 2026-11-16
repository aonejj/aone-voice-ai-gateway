// ===== RTCAudioTickScheduler.cc =====
#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif

#define LOG_TAG "RTCAudioTickScheduler"
#include <utils/Log.h>

#include <algorithm>
#include <time.h>
#include <pthread.h>

#include "../RTCClient/RTCAudioSenderTickInterface.h"
#include "RTCAudioTickScheduler.h"


RTCAudioTickScheduler::RTCAudioTickScheduler() {
}

RTCAudioTickScheduler::~RTCAudioTickScheduler() {
    Stop();
}

int64_t RTCAudioTickScheduler::_now_us() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000000LL + ts.tv_nsec / 1000;
}

void RTCAudioTickScheduler::_sleep_until_us(int64_t abs_us) {
    struct timespec ts;
    ts.tv_sec = abs_us / 1000000LL;
    ts.tv_nsec = (abs_us % 1000000LL) * 1000;

    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, nullptr) == EINTR) {
    }
}

bool RTCAudioTickScheduler::Start(int worker_count) {
    std::lock_guard<std::mutex> g(_state_mutex);
    if (_started || worker_count < 1) {
        return false;
    }

    _workers.reserve(worker_count);
    for (int i = 0; i < worker_count; i++) {
        _workers.push_back(std::make_unique<Worker>());
    }
    _started = true;

    for (auto& w : _workers) {
        w->running.store(true);
        Worker* raw = w.get();
        w->thread = std::thread([this, raw]() { _worker_loop(raw); });
    }

    ALOGI("started: workers=%d", worker_count);
    return true;
}

void RTCAudioTickScheduler::Stop() {
    std::vector<std::unique_ptr<Worker>> workers;
    {
        std::lock_guard<std::mutex> g(_state_mutex);
        if (!_started) {
            return;
        }
        _started = false;
        workers.swap(_workers);
    }

    for (auto& w : workers) {
        w->running.store(false);
    }
    for (auto& w : workers) {
        if (w->thread.joinable()) {
            w->thread.join();
        }
    }

    ALOGI("stopped");
}

bool RTCAudioTickScheduler::Register(RTCAudioSenderTickInterface* sender) {
    ALOGV("Register");
    if (sender == nullptr) {
        return false;
    }

    std::lock_guard<std::mutex> g(_state_mutex);
    if (!_started || _workers.empty()) {
        return false;
    }

    Worker* target = nullptr;
    size_t min_count = SIZE_MAX;
    for (auto& w : _workers) {
        std::lock_guard<std::mutex> wl(w->mutex);

        if (std::find(w->senders.begin(), w->senders.end(), sender) != w->senders.end()) {
            return true;
        }
        if (w->senders.size() < min_count) {
            min_count = w->senders.size();
            target = w.get();
        }
    }

    std::lock_guard<std::mutex> wl(target->mutex);
    target->senders.push_back(sender);
    return true;
}

void RTCAudioTickScheduler::Unregister(RTCAudioSenderTickInterface* sender) {
    ALOGV("Unregister");
    if (sender == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> g(_state_mutex);
    for (auto& w : _workers) {
        std::lock_guard<std::mutex> wl(w->mutex);  
        auto it = std::find(w->senders.begin(), w->senders.end(), sender);
        if (it != w->senders.end()) {
            w->senders.erase(it);
            return;
        }
    }
}

RTCAudioTickScheduler::Stats RTCAudioTickScheduler::GetStats() {
    Stats s;
    std::lock_guard<std::mutex> g(_state_mutex);
    for (auto& w : _workers) {
        s.ticks += w->ticks.load();
        s.late_ticks += w->late_ticks.load();
        s.max_tick_cost_us = std::max(s.max_tick_cost_us, w->max_tick_cost_us.load());
        s.max_lateness_us = std::max(s.max_lateness_us, w->max_lateness_us.load());
        std::lock_guard<std::mutex> wl(w->mutex);
        s.sender_count += w->senders.size();
    }
    return s;
}

void RTCAudioTickScheduler::_worker_loop(Worker* w) {
    pthread_setname_np(pthread_self(), "TickScheduler");
    int64_t next_us = _now_us();

    while (w->running.load()) {
        next_us += kTickIntervalUs;
        _sleep_until_us(next_us);

        if (!w->running.load()) {
            break;
        }

        const int64_t start_us = _now_us();
        const int64_t lateness_us = start_us - next_us;

        if (lateness_us > w->max_lateness_us.load()) {
            w->max_lateness_us.store(lateness_us);
        }

        if (lateness_us > kResyncThresholdUs) {
            w->late_ticks.fetch_add(1);
            next_us = start_us;
        }

        {
            std::lock_guard<std::mutex> lock(w->mutex);
            for (RTCAudioSenderTickInterface* s : w->senders) {
                s->OnTick();
            }
        }

        const int64_t cost_us = _now_us() - start_us;
        if (cost_us > w->max_tick_cost_us.load()) {
            w->max_tick_cost_us.store(cost_us);
        }
        w->ticks.fetch_add(1);

        if (cost_us > kTickIntervalUs) {
            ALOGW("tick overrun: cost=%lldus senders=%zu",
                  (long long)cost_us, w->senders.size());
        }
    }
}
