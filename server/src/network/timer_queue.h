#pragma once

#include "net_callback.h"
#include "common/platform.h"
#include <utility> // For std::pair
#include <set>     // For std::set
#include <cstdint> // For int64_t

namespace w_network
{
    class EventLoop;
    class Timer;
    class TimerId;

    class TimerQueue
    {
    public:
        TimerQueue(EventLoop* loop);
        ~TimerQueue();
        //interval单位是微妙
        TimerId add_timer(const TimerCallback& cb, Timestamp when, int64_t interval, int64_t repeat_count);

        TimerId add_timer(TimerCallback&& cb, Timestamp when, int64_t interval, int64_t repeat_count);

        void remove_timer(TimerId timer_id);

        void cancel(TimerId timer_id, bool off);

        // called when timerfd alarms
        void do_timer();

    private:
        TimerQueue(const TimerQueue& rhs) = delete;
        TimerQueue& operator=(const TimerQueue& rhs) = delete;

        typedef std::pair<Timestamp, Timer*>    Entry;
        typedef std::set<Entry>                 TimerList;
        typedef std::pair<Timer*, int64_t>      ActiveTimer;
        typedef std::set<ActiveTimer>           ActiveTimerSet;

        void add_timer_in_loop(Timer* timer);
        void remove_timer_in_loop(TimerId timer_id);
        void cancel_timer_in_loop(TimerId timer_id, bool off);

        void insert(Timer* timer);

        private:
        EventLoop* loop_;
        TimerList           timers_; 
    };
}