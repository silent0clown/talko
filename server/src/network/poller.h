#pragma once

#include <vector>
#include "common/whisp_timestamp.h"

namespace w_network
{
    class Channel;

    class Poller
    {
    public:
        Poller();
        ~Poller();

    public:
        typedef std::vector<Channel*> ChannelList;

        virtual Timestamp poll(int timeout_ms, ChannelList* active_channels) = 0;
        virtual bool update_channel(Channel* channel) = 0;
        virtual void remove_channel(Channel* channel) = 0;

        virtual bool has_channel(Channel* channel) const = 0;
    };
}
