#pragma once

#include <functional>

#include "channel.h"
#include "w_sockets.h"

namespace w_network
{
    class EventLoop;
    class InetAddress;

    class Acceptor
    {
    public:
        typedef std::function<void(int sockfd, const InetAddress&)> NewConnectionCallback;

        Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
        ~Acceptor();

        //设置新连接到来的回调函数
        void set_new_conn_callback(const NewConnectionCallback& cb)
        {
            new_conn_callback_ = cb;
        }

        bool listenning() const { return listening_; }
        void listen();

    private:
        void _handle_read();

    private:
        EventLoop* loop_;
        Socket                accept_socket_;
        Channel               accept_channel_;
        NewConnectionCallback new_conn_callback_;
        bool                  listening_;

#ifndef WIN32
        int                   idle_fd_;
#endif
    };
}
