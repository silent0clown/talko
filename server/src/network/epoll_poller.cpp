// #include "net_callback.h"
#include "epoll_poller.h"

#ifndef WIN32
#include "log/whisp_log.h"
#include "channel.h" // Include the header where Channel is fully defined
#include "event_loop.h" // Include the header where EventLoop is fully defined
#include <memory.h>

using namespace w_network;

namespace {
    const int new_ = -1;
    const int added_ = 1;
    const int deleted_ = 2;
}

EpollPoller::EpollPoller(EventLoop* loop)
    : epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(init_event_list_size_),
    owner_loop_(loop)
{
    if (epollfd_ < 0)
    {
        WHISP_LOG_FALTAL("EPollPoller::EPollPoller");
    }
}

EpollPoller::~EpollPoller()
{
    ::close(epollfd_);
}

bool EpollPoller::has_channel(Channel* channel) const
{
    assert_in_loop_thread();
    ChannelMap::const_iterator it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

void EpollPoller::assert_in_loop_thread() const
{
    owner_loop_->assert_in_loop_thread();
}

Timestamp EpollPoller::poll(int timeout_ms, ChannelList* active_channels)
{
    int numEvents = ::epoll_wait(epollfd_,
        &*events_.begin(),
        static_cast<int>(events_.size()),
        timeout_ms);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        //LOG_TRACE << numEvents << " events happended";
        _fill_active_channels(numEvents, active_channels);
        if (static_cast<size_t>(numEvents) == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        //LOG_TRACE << " nothing happended";
    }
    else
    {
        // error happens, log uncommon ones
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            WHISP_LOG_SYSERROR("EPollPoller::poll()");
        }
    }
    return now;
}

void EpollPoller::_fill_active_channels(int events_num, ChannelList* active_channels) const
{
    for (int i = 0; i < events_num; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        int fd = channel->fd();
        ChannelMap::const_iterator it = channels_.find(fd);
        if (it == channels_.end() || it->second != channel)
            return;
        channel->set_revents(events_[i].events);
        active_channels->push_back(channel);
    }
}

bool EpollPoller::update_channel(Channel* channel)
{
    assert_in_loop_thread();
    WHISP_LOG_DEBUG("fd = %d  events = %d", channel->fd(), channel->events());
    const int index = channel->index();
    if (index == new_ || index == deleted_)
    {
        int fd = channel->fd();
        if (index == new_)
        {
            if (channels_.find(fd) != channels_.end())
            {
                WHISP_LOG_DEBUG("fd = %d  must not exist in channels_", fd);
                return false;
            }


            channels_[fd] = channel;
        }
        else // index == deleted_
        {
            if (channels_.find(fd) == channels_.end())
            {
                WHISP_LOG_ERROR("fd = %d  must exist in channels_", fd);
                return false;
            }

            //assert(channels_[fd] == channel);
            if (channels_[fd] != channel)
            {
                WHISP_LOG_ERROR("current channel is not matched current fd, fd = %d", fd);
                return false;
            }
        }
        channel->set_index(added_);

        return _update(XEPOLL_CTL_ADD, channel);
    }
    else
    {
        // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        if (channels_.find(fd) == channels_.end() || channels_[fd] != channel || index != added_)
        {
            WHISP_LOG_ERROR("current channel is not matched current fd, fd = %d, channel = 0x%x", fd, channel);
            return false;
        }

        if (channel->is_event_none())
        {
            if (_update(XEPOLL_CTL_DEL, channel))
            {
                channel->set_index(deleted_);
                return true;
            }
            return false;
        }
        else
        {
            return _update(XEPOLL_CTL_MOD, channel);
        }
    }
}


void EpollPoller::remove_channel(Channel* channel)
{
    assert_in_loop_thread();
    int fd = channel->fd();

    if (channels_.find(fd) == channels_.end() || channels_[fd] != channel || !channel->is_event_none())
        return;

    int index = channel->index();
    if (index != added_ && index != deleted_)
        return;

    size_t n = channels_.erase(fd);
    if (n != 1)
        return;

    if (index == added_)
    {
        _update(XEPOLL_CTL_DEL, channel);
    }
    channel->set_index(new_);
}

bool EpollPoller::_update(int operation, Channel* channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == XEPOLL_CTL_DEL)
        {
            WHISP_LOG_ERROR("epoll_ctl op=%d fd=%d, epollfd=%d, errno=%d, errorInfo: %s", operation, fd, epollfd_, errno, strerror(errno));
        }
        else
        {
            WHISP_LOG_ERROR("epoll_ctl op=%d fd=%d, epollfd=%d, errno=%d, errorInfo: %s", operation, fd, epollfd_, errno, strerror(errno));
        }

        return false;
    }

    return true;
}

#endif