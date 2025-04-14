#include "timer.h"
#include "common/whisp_timestamp.h"

using namespace w_network;
std::atomic<int64_t> Timer::created_num_;

Timer::Timer(const TimerCallback& cb, Timestamp when, int64_t interval, int64_t repeat_count/* = -1*/)
    : callback_(cb),
    expiration_(when),
    interval_(interval),
    repeat_count_(repeat_count),
    sequence_(++created_num_),
    canceled_(false)
{ }

Timer::Timer(TimerCallback&& cb, Timestamp when, int64_t interval)
    : callback_(std::move(cb)),
    expiration_(when),
    interval_(interval),
    repeat_count_(-1),
    sequence_(++created_num_),
    canceled_(false)
{ }

void Timer::run()
{
    if (canceled_)
        return;

    callback_();

    if (repeat_count_ != -1)
    {
        --repeat_count_;
        if (repeat_count_ == 0)
        {
            return;
        }
    }

    expiration_ += interval_;
}