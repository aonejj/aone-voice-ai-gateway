#ifndef __RTC_HANDLER_H__
#define __RTC_HANDLER_H__

#include <cassert>
#include <chrono>
#include <future>
#include <memory>
#include <utility>
#include <type_traits>

#include "RTCLooper.h"

class RTCHandler : public std::enable_shared_from_this<RTCHandler> {
public:
    virtual ~RTCHandler() = default;

protected:
    explicit RTCHandler(std::shared_ptr<RTCLooper> looper) : looper_(std::move(looper)) {
        assert(looper_ && "Handler needs a Looper");
    }
 
    template <typename F>
    void Post(F f) {
        PostDelayed(std::chrono::milliseconds(0), std::move(f));
    }
 
    template <typename F>
    void PostDelayed(std::chrono::milliseconds delay, F f) {
        looper_->PostDelayed(delay, [w = weak_from_this(), f = std::move(f)]() mutable {
            if (auto self = w.lock()) {
                f();
            }  
        });
    }

    template <typename F>
    status_t PostAndWait(F f) {
        using R = std::invoke_result_t<F&>;
        static_assert(std::is_void_v<R> || std::is_convertible_v<R, status_t>,
                        "PostAndWait: f must return void or status_t");

        if (IsOnLooperThread()) {
            if constexpr (std::is_void_v<R>) {
                 f(); 
                 return OK; 
            } else { 
                return f(); 
            }
        }

        auto promise = std::make_shared<std::promise<status_t>>();
        std::future<status_t> result = promise->get_future();
        Post([promise = std::move(promise), f = std::move(f)]() mutable {
            if constexpr (std::is_void_v<R>) { 
                f(); 
                promise->set_value(OK); 
            } else { 
                promise->set_value(f()); 
            }
        });

        try {
            return result.get();
        } catch (const std::future_error&) {
            return DEAD_OBJECT;
        }
    }    
 
    RTCLooper& looper() const { return *looper_; }
    bool IsOnLooperThread() const { return looper_->IsCurrent(); }

private:
    std::shared_ptr<RTCLooper> looper_;

private:
    RTCHandler(const RTCHandler&) = delete;
    RTCHandler& operator=(const RTCHandler&) = delete;    
};

#endif // __RTC_HANDLER_H__