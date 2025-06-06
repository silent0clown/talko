#include "timer.h"
#include "timer_id.h"
#include "timer_queue.h"
#include "event_loop.h"
#include "log/whisp_log.h"
#include <functional>

namespace w_network
{
    //namespace detail
    //{

    //    int createTimerfd()
    //    {
    //        int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    //        if (timerfd < 0)
    //        {
    //            LOG_SYSFATAL << "Failed in timerfd_create";
    //        }
    //        return timerfd;
    //    }

    //    struct timespec howMuchTimeFromNow(Timestamp when)
    //    {
    //        int64_t microseconds = when.microSecondsSinceEpoch()
    //            - Timestamp::now().microSecondsSinceEpoch();
    //        if (microseconds < 100)
    //        {
    //            microseconds = 100;
    //        }
    //        struct timespec ts;
    //        ts.tv_sec = static_cast<time_t>(
    //            microseconds / Timestamp::kMicroSecondsPerSecond);
    //        ts.tv_nsec = static_cast<long>(
    //            (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    //        return ts;
    //    }

    //    void readTimerfd(int timerfd, Timestamp now)
    //    {
    //        uint64_t howmany;
    //        ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    //        LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    //        if (n != sizeof howmany)
    //        {
    //            LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    //        }
    //    }

    //    void resetTimerfd(int timerfd, Timestamp expiration)
    //    {
    //        // wake up loop by timerfd_settime()
    //        struct itimerspec newValue;
    //        struct itimerspec oldValue;
    //        bzero(&newValue, sizeof newValue);
    //        bzero(&oldValue, sizeof oldValue);
    //        newValue.it_value = howMuchTimeFromNow(expiration);
    //        int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    //        if (ret)
    //        {
    //            LOG_SYSERR << "timerfd_settime()";
    //        }
    //    }

    //}
}

using namespace w_network;
//using namespace net::detail;

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
    /*timerfd_(createTimerfd()),
    timerfdChannel_(loop, timerfd_),*/
    timers_()
    //callingExpiredTimers_(false)
{
    //timerfdChannel_.setReadCallback(
    //    std::bind(&TimerQueue::handleRead, this));
    // we are always reading the timerfd, we disarm it with timerfd_settime.
    //将timerfd挂到epollfd上
    //timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    //timerfdChannel_.disableAll();
    //timerfdChannel_.remove();
    //::close(timerfd_);
    // do not remove channel, since we're in EventLoop::dtor();
    for (TimerList::iterator it = timers_.begin(); it != timers_.end(); ++it)
    {
        delete it->second;
    }
}

TimerId TimerQueue::add_timer(const TimerCallback& cb, Timestamp when, int64_t interval, int64_t repeat_count)
{
    Timer* timer = new Timer(cb, when, interval);
    loop_->run_in_loop(std::bind(&TimerQueue::add_timer_in_loop, this, timer));
    return TimerId(timer, timer->sequence());
}

TimerId TimerQueue::add_timer(TimerCallback&& cb, Timestamp when, int64_t interval, int64_t repeat_count)
{
    Timer* timer = new Timer(std::move(cb), when, interval, repeat_count);
    loop_->run_in_loop(std::bind(&TimerQueue::add_timer_in_loop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::remove_timer(TimerId timer_id)
{
    loop_->run_in_loop(std::bind(&TimerQueue::remove_timer_in_loop, this, timer_id));
}

void TimerQueue::cancel(TimerId timer_id, bool off)
{
    loop_->run_in_loop(std::bind(&TimerQueue::cancel_timer_in_loop, this, timer_id, off));
}

void TimerQueue::do_timer()
{
    loop_->assert_in_loop_thread();

    Timestamp now(Timestamp::now());

    for (auto iter = timers_.begin(); iter != timers_.end(); )
    {
        if (iter->second->expiration() <= now)
        {
            //LOGD("time: %lld", iter->second->expiration().microSecondsSinceEpoch());
            iter->second->run();
            if (iter->second->get_repeat_count() == 0)
            {
                iter = timers_.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
        else
        {
            break;
        }
    }


    //readTimerfd(timerfd_, now);

    //std::vector<Entry> expired = getExpired(now);

    //callingExpiredTimers_ = true;
    //cancelingTimers_.clear();
    //// safe to callback outside critical section
    //for (std::vector<Entry>::iterator it = expired.begin();
    //    it != expired.end(); ++it)
    //{
    //    it->second->run();
    //}
    //callingExpiredTimers_ = false;

    //reset(expired, now);
}

void TimerQueue::add_timer_in_loop(Timer* timer)
{
    loop_->assert_in_loop_thread();
    /*bool earliestChanged = */insert(timer);

    //if (earliestChanged)
    //{
    //    resetTimerfd(timerfd_, timer->expiration());
    //}
}

void TimerQueue::remove_timer_in_loop(TimerId timer_id)
{
    loop_->assert_in_loop_thread();
    //assert(timers_.size() == activeTimers_.size());
    //ActiveTimer timer(timerId.timer_, timerId.sequence_);
    //ActiveTimerSet::iterator it = activeTimers_.find(timer);

    Timer* timer = timer_id.timer_;
    for (auto iter = timers_.begin(); iter != timers_.end(); ++iter)
    {
        if (iter->second == timer)
        {
            timers_.erase(iter);
            break;
        }
    }
}

void TimerQueue::cancel_timer_in_loop(TimerId timerId, bool off)
{
    loop_->assert_in_loop_thread();

    Timer* timer = timerId.timer_;
    for (auto iter = timers_.begin(); iter != timers_.end(); ++iter)
    {
        if (iter->second == timer)
        {
            iter->second->cancel(off);
            break;
        }
    }

    ////assert(timers_.size() == activeTimers_.size());
    //ActiveTimer timer(timerId.timer_, timerId.sequence_);
    //ActiveTimerSet::iterator it = activeTimers_.find(timer);
    //if (it != activeTimers_.end())
    //{
    //    size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
    //    //assert(n == 1); (void)n;
    //    delete it->first; // FIXME: no delete please
    //    activeTimers_.erase(it);
    //}
    //else if (callingExpiredTimers_)
    //{
    //    cancelingTimers_.insert(timer);
    //}
    ////assert(timers_.size() == activeTimers_.size());
}

//std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
//{
//    assert(timers_.size() == activeTimers_.size());
//    std::vector<Entry> expired;
//    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
//    TimerList::iterator end = timers_.lower_bound(sentry);
//    assert(end == timers_.end() || now < end->first);
//    std::copy(timers_.begin(), end, back_inserter(expired));
//    timers_.erase(timers_.begin(), end);
//
//    for (std::vector<Entry>::iterator it = expired.begin();
//        it != expired.end(); ++it)
//    {
//        ActiveTimer timer(it->second, it->second->sequence());
//        size_t n = activeTimers_.erase(timer);
//        assert(n == 1); (void)n;
//    }
//
//    assert(timers_.size() == activeTimers_.size());
//    return expired;
//}

//void TimerQueue::reset(const std::vector<Entry> & expired, Timestamp now)
//{
//    Timestamp nextExpire;
//
//    for (std::vector<Entry>::const_iterator it = expired.begin();
//        it != expired.end(); ++it)
//    {
//        ActiveTimer timer(it->second, it->second->sequence());
//        if (it->second->getRepeatCount()
//            && cancelingTimers_.find(timer) == cancelingTimers_.end())
//        {
//            it->second->restart(now);
//            insert(it->second);
//        }
//        else
//        {
//            // FIXME move to a free list
//            delete it->second; // FIXME: no delete please
//        }
//    }
//
//    if (!timers_.empty())
//    {
//        nextExpire = timers_.begin()->second->expiration();
//    }
//
//    if (nextExpire.valid())
//    {
//        resetTimerfd(timerfd_, nextExpire);
//    }
//}

void TimerQueue::insert(Timer* timer)
{
    loop_->assert_in_loop_thread();
    //assert(timers_.size() == activeTimers_.size());
    bool earliest_changed = false;
    Timestamp when = timer->expiration();
    //TimerList::iterator it = timers_.begin();
    //if (it == timers_.end() || when < it->first)
    //{
    //    earliestChanged = true;
    //}
    //{
    /*std::pair<TimerList::iterator, bool> result = */timers_.insert(Entry(when, timer));
    //assert(result.second); (void)result;
//}
//{
//    std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
    //assert(result.second); (void)result;
//}

//assert(timers_.size() == activeTimers_.size());
//return earliestChanged;
}
