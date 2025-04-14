#include "event_loop_threadpool.h"
#include "event_loop_thread.h"
#include "event_loop.h"
#include "net_callback.h"
#include <stdio.h>
#include <assert.h>
#include <sstream>
#include <string>

using namespace w_network;


EventLoopThreadPool::EventLoopThreadPool()
    : base_loop_(NULL),
    started_(false),
    num_threads_(0),
    next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // Don't delete loop, it's stack variable
}

void EventLoopThreadPool::init(EventLoop* base_loop, int threads_num)
{
    num_threads_ = threads_num;
    base_loop_ = base_loop;
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
    //assert(baseLoop_);
    if (base_loop_ == NULL)
        return;

    //assert(!started_);
    if (started_)
        return;

    base_loop_->assert_in_loop_thread();

    started_ = true;

    for (int i = 0; i < num_threads_; ++i)
    {
        char buf[128];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);

        std::unique_ptr<EventLoopThread> t(new EventLoopThread(cb, buf));
        //EventLoopThread* t = new EventLoopThread(cb, buf);        
        loops_.push_back(t->start_loop());
        threads_.push_back(std::move(t));
    }
    if (num_threads_ == 0 && cb)
    {
        cb(base_loop_);
    }
}

void EventLoopThreadPool::stop()
{
    for (auto& iter : threads_)
    {
        iter->stop_loop();
    }
}

EventLoop* EventLoopThreadPool::get_next_loop()
{
    base_loop_->assert_in_loop_thread();
    //assert(started_);
    if (!started_)
        return NULL;

    EventLoop* loop = base_loop_;

    if (!loops_.empty())
    {
        // round-robin
        loop = loops_[next_];
        ++next_;
        if (size_t(next_) >= loops_.size())
        {
            next_ = 0;
        }
    }
    return loop;
}

EventLoop* EventLoopThreadPool::get_loop_for_hash(size_t hashCode)
{
    base_loop_->assert_in_loop_thread();
    EventLoop* loop = base_loop_;

    if (!loops_.empty())
    {
        loop = loops_[hashCode % loops_.size()];
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::get_all_loops()
{
    base_loop_->assert_in_loop_thread();
    if (loops_.empty())
    {
        return std::vector<EventLoop*>(1, base_loop_);
    }
    else
    {
        return loops_;
    }
}

const std::string EventLoopThreadPool::info() const
{
    std::stringstream ss;
    ss << "print threads id info " << std::endl;
    for (size_t i = 0; i < loops_.size(); i++)
    {
        ss << i << ": id = " << loops_[i]->get_thread_id() << std::endl;
    }
    return ss.str();
}
