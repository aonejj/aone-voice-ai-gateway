#ifndef __RTC_LOOPER_H__
#define __RTC_LOOPER_H__

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

class RTCLooper {
public:
    using Clock = std::chrono::steady_clock;
    using Task = std::function<void()>;
    
    explicit RTCLooper(std::string name = "Looper") 
     : _state(std::make_shared<State>(std::move(name))) {
        _thread = std::thread([s = _state]
        {
            Run(s);
        });
    }
  
    ~RTCLooper() { Stop(); }

    void Post(Task t) { _post_at(Clock::now(), std::move(t)); }
    void PostDelayed(std::chrono::milliseconds delay, Task t) {
        _post_at(Clock::now() + delay, std::move(t));
    }

    bool IsCurrent() const { return _state->tid.load(std::memory_order_acquire) == std::this_thread::get_id(); }

    void Stop() {
        std::priority_queue<Item, std::vector<Item>, std::greater<Item>> discarded;
        {
            std::lock_guard<std::mutex> lock(_state->m);
            _state->stop = true;
            discarded.swap(_state->q);
        }

        _state->cv.notify_all();

        if(_thread.joinable()) {
            if(IsCurrent()) {
                _thread.detach();
            } else {
                _thread.join();
            }
        }
    }

private:
    struct Item {
        Clock::time_point when;
        uint64_t seq;
        Task task;
        bool operator>(const Item& o) const {
        return when != o.when ? when > o.when : seq > o.seq;
        }
    };    

    struct State {
        explicit State(std::string name) : _name(std::move(name)) {}
        const std::string _name;
        std::atomic<std::thread::id> tid{std::thread::id()};
        std::mutex m;
        std::condition_variable cv;
        std::priority_queue<Item, std::vector<Item>, std::greater<Item>> q;
        uint64_t next_seq = 0;
        bool stop = false;
    };

private:
    void _post_at(Clock::time_point when, Task t) {
        const std::shared_ptr<State> state = _state;
        {
            std::lock_guard<std::mutex> lock(state->m);
            if(state->stop) {
                return;
            }
            state->q.push(Item{when, state->next_seq++, std::move(t)});
        }
        state->cv.notify_one();
    }

    static void Run(const std::shared_ptr<State>& s) {
        s->tid.store(std::this_thread::get_id(), std::memory_order_release);
        std::unique_lock<std::mutex> lock(s->m);
        while (!s->stop) {
            if (s->q.empty()) { 
                s->cv.wait(lock); 
                continue; 
            }
            const auto when = s->q.top().when;
            if (Clock::now() < when) { 
                s->cv.wait_until(lock, when); 
                continue; 
            }
            Task t = std::move(const_cast<Item&>(s->q.top()).task);
            s->q.pop();
            lock.unlock();
            t();
            t = nullptr;
            lock.lock();
        }
        lock.unlock();
        s->tid.store(std::thread::id(), std::memory_order_release);        
    }

private:
    const std::shared_ptr<State> _state;
    std::thread _thread;

private:
    RTCLooper(const RTCLooper&) = delete;
    RTCLooper& operator=(const RTCLooper&) = delete;   
};

#endif