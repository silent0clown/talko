#include "channel.h"
#include <sstream>

#include "common/platform.h"
#include "log/whisp_log.h"
#include "poller.h"
#include "event_loop.h"

using namespace w_network;
namespace w_network
{
    const int Channel::none_event_ = 0;
    const int Channel::read_event_ = POLLIN | POLLPRI;  // 修正：XPOLLIN → POLLIN
    const int Channel::write_event_ = POLLOUT;           // 修正：XPOLLOUT → POLLOUT

    Channel::Channel(EventLoop* loop, int fd)
        : loop_(loop),
          fd_(fd),
          events_(0),
          revents_(0),
          index_(-1),
          read_callback_(nullptr),
          write_callback_(nullptr),
          close_callback_(nullptr),
          error_callback_(nullptr)
    {
    }

    Channel::~Channel()
    {
    }

    bool Channel::enable_reading()
    {
        events_ |= read_event_;
        return _update();
    }

    bool Channel::disable_reading()
    {
        events_ &= ~read_event_;
        return _update();
    }

    bool Channel::enable_writing()
    {
        events_ |= write_event_;
        return _update();
    }

    bool Channel::disable_writing()
    {
        events_ &= ~write_event_;
        return _update();
    }

    bool Channel::disable_all()
    {
        events_ = none_event_;
        return _update();
    }

    bool Channel::_update()
    {
        return loop_->update_channel(this);  // 假设 EventLoop 有 updateChannel 方法
    }

    void Channel::remove()
    {
        if (is_event_none())
            loop_->remove_channel(this);  // 假设 EventLoop 有 removeChannel 方法
    }

    void Channel::handle_event(Timestamp receive_time)
    {
        WHISP_LOG_DEBUG(revents_2_string().c_str());

        // 处理挂断事件（POLLHUP 且无数据可读）
        if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
        {
            if (close_callback_)
                close_callback_();
        }

        // 处理非法文件描述符（POLLNVAL）
        if (revents_ & POLLNVAL)
        {
            WHISP_LOG_WARN("Channel::handle_event() POLLNVAL");
        }

        // 处理错误事件（POLLERR 或 POLLNVAL）
        if (revents_ & (POLLERR | POLLNVAL))
        {
            if (error_callback_)
                error_callback_();
        }

        // 处理读事件（POLLIN | POLLPRI | POLLRDHUP）
        if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
        {
            if (read_callback_)
                read_callback_(receive_time);
        }

        // 处理写事件（POLLOUT）
        if (revents_ & POLLOUT)
        {
            if (write_callback_)
                write_callback_();
        }
    }

    std::string Channel::revents_2_string() const
    {
        std::ostringstream oss;
        oss << fd_ << ": ";
        if (revents_ & POLLIN)
            oss << "IN ";
        if (revents_ & POLLPRI)
            oss << "PRI ";
        if (revents_ & POLLOUT)
            oss << "OUT ";
        if (revents_ & POLLHUP)
            oss << "HUP ";
        if (revents_ & POLLRDHUP)
            oss << "RDHUP ";
        if (revents_ & POLLERR)
            oss << "ERR ";
        if (revents_ & POLLNVAL)
            oss << "NVAL ";

        return oss.str();
    }
}