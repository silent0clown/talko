#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <string>

namespace w_network
{
    class EventLoop;
    class EventLoopThread;

    class EventLoopThreadPool
    {
    public:
        typedef std::function<void(EventLoop*)> ThreadInitCallback;

        EventLoopThreadPool();
        ~EventLoopThreadPool();

        void init(EventLoop* base_loop, int num_threads);
        void start(const ThreadInitCallback& cb = ThreadInitCallback());

        void stop();

        /// round-robin
        EventLoop* get_next_loop();

        /// with the same hash code, it will always return the same EventLoop
        EventLoop* get_loop_for_hash(size_t hash_code);

        std::vector<EventLoop*> get_all_loops();

        bool started() const
        {
            return started_;
        }

        const std::string& name() const
        {
            return name_;
        }

        const std::string info() const;

    private:

        EventLoop* base_loop_;
        std::string                                     name_;
        bool                                            started_;
        int                                             num_threads_;
        int                                             next_;
        std::vector<std::unique_ptr<EventLoopThread> >  threads_;
        std::vector<EventLoop*>                         loops_;
    };

}
