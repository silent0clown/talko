#pragma once

#include <mutex>
#include <condition_variable> 
#include <thread>
#include <string>
#include <functional>

namespace w_network
{
    class EventLoop;

    class EventLoopThread
    {
    public:
        typedef std::function<void(EventLoop*)> ThreadInitCallback;

        EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(), const std::string& name = "");
        ~EventLoopThread();
        EventLoop* start_loop();
        void stop_loop();

    private:
        void _thread_func();

        EventLoop* loop_;
        bool                         exiting_;
        std::unique_ptr<std::thread> thread_;
        std::mutex                   mutex_;
        std::condition_variable      cond_;
        ThreadInitCallback           callback_;
    };

}
