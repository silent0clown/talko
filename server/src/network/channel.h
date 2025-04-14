#pragma oncce

#include "common/whisp_timestamp.h"
#include <functional>

namespace w_network
{
    class EventLoop;
    class Channel {
    public:
        typedef std::function<void()> EventCallback;
        typedef std::function<void(Timestamp)> ReadEventCallback;

        Channel(EventLoop* loop, int fd);
        ~Channel();

        void handle_event(Timestamp receive_time);
        void set_read_callback(const ReadEventCallback& cb)
        {
            read_callback_ = cb;
        }
        void set_write_callback(const EventCallback& cb)
        {
            write_callback_ = cb;
        }
        void set_close_callback(const EventCallback& cb)
        {
            close_callback_ = cb;
        }
        void set_error_callback(const EventCallback& cb)
        {
            error_callback_ = cb;
        }

        int fd() const { return fd_; }
        int events() const { return events_; }
        void set_revents(int revt) { revents_ = revt; }
        void add_revents(int revt) { revents_ |= revt; }
        // int revents() const { return revents_; }
        bool is_event_none() const { return events_ == none_event_; }

        bool enable_reading();
        bool disable_reading();
        bool enable_writing();
        bool disable_writing();
        bool disable_all();

        bool is_writing() const { return events_ & write_event_; }

        int index() { return index_; }
        void set_index(int idx) { index_ = idx; }

        std::string revents_2_string() const;

        EventLoop* owner_loop() { return loop_; }
        void remove();

    private:
        bool _update();

        static const int            none_event_;
        static const int            read_event_;
        static const int            write_event_;

        EventLoop* loop_;
        const int                   fd_;
        int                         events_;
        int                         revents_;
        int                         index_;

        ReadEventCallback           read_callback_;
        EventCallback               write_callback_;
        EventCallback               close_callback_;
        EventCallback               error_callback_;
    };
}