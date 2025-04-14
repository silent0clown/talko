#pragma once

#include "net_callback.h"
#include "timer_id.h"
#include "timer_queue.h"
#include "common/whisp_timestamp.h"
#include "common/platform.h"
#include <mutex>
#include <thread>

namespace w_network
{
    class EventLoop;
    class Channel;
    class Poller;
    class CTimerHeap;

    class EventLoop {
    public:
        typedef std::function<void()> Functor;
        EventLoop();
        ~EventLoop();

        void loop();    // 不要在类成员函数前加 et_ 前缀。类作用域 + 命名空间已足够区分。
        void quit();
        Timestamp poll_return_time() const { return poll_return_time_; }
        int64_t iteration() const { return iteration_; }
      
        void run_in_loop(const Functor& cb);
        void queue_in_loop(const Functor& cb);
      
        TimerId run_at(const Timestamp& time, const TimerCallback& cb);
        TimerId run_after(int64_t delay, const TimerCallback& cb);
        TimerId run_every(int64_t interval, const TimerCallback& cb);
        void cancel(TimerId timer_id, bool off);
        void remove(TimerId timer_id);
      
        TimerId run_at(const Timestamp& time, TimerCallback&& cb);
        TimerId run_after(int64_t delay, TimerCallback&& cb);
        TimerId run_every(int64_t interval, TimerCallback&& cb);
      
        void set_frame_functor(const Functor& cb);
      
        bool update_channel(Channel* channel);
        void remove_channel(Channel* channel);
        bool has_channel(Channel* channel);
      
        void assert_in_loop_thread() {
          if (!is_in_loop_thread()) {
            abort_not_in_loop_thread();
          }
        }
        bool is_in_loop_thread() const { return thread_id_ == std::this_thread::get_id(); }
        bool event_handling() const { return event_handling_; }
      
        const std::thread::id get_thread_id() const { return thread_id_; }
      
       private:
        bool create_wakeup_fd();
        bool wakeup();
        void abort_not_in_loop_thread();
        bool handle_read();
        void do_other_tasks();
      
        void print_active_channels() const;
      
        using ChannelList = std::vector<Channel*>;
      
        bool looping_;
        bool quit_;
        bool event_handling_;
        bool doing_other_tasks_;
        const std::thread::id thread_id_;
        Timestamp poll_return_time_;
        std::unique_ptr<Poller> poller_;
        std::unique_ptr<TimerQueue> timer_queue_;
        int64_t iteration_;
      
      #ifdef WIN32
        SOCKET wakeup_fd_send_;
        SOCKET wakeup_fd_listen_;
        SOCKET wakeup_fd_recv_;
      #else
        SOCKET wakeup_fd_;
      #endif
      
        std::unique_ptr<Channel> wakeup_channel_;
      
        ChannelList active_channels_;
        Channel* current_active_channel_;
      
        std::mutex mutex_;
        std::vector<Functor> pending_functors_;
      
        Functor frame_functor_;
    };
      
}  // namespace w_network