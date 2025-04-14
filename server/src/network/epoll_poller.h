#pragma once

#ifndef WIN32

#include <vector>
#include <map>

#include "common/platform.h"
#include "poller.h"

struct epoll_event;

namespace w_network
{
    class EventLoop;

    class EpollPoller : public Poller{
    public:
        EpollPoller(EventLoop* loop);
        virtual ~EpollPoller();

        virtual Timestamp poll(int timeout_ms, ChannelList* active_channels);
        virtual bool update_channel(Channel* channel);
        virtual void remove_channel(Channel* channel);

        virtual bool has_channel(Channel* channel) const;

        void assert_in_loop_thread() const;

    private:
        static const int init_event_list_size_ = 16;

        void _fill_active_channels(int events_num, ChannelList* active_channels) const;
        bool _update(int operation, Channel* channel);
    
    private:
        typedef std::vector<struct epoll_event> EventList;
        int  epollfd_;
        EventList events_;
        typedef std::map<int, Channel*> ChannelMap;
        ChannelMap channels_;
        EventLoop* owner_loop_;
    };
}



#endif