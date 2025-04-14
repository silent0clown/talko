#pragma once

#include "tcp_connect.h"

#include <atomic>
#include <map>


namespace w_network
{
    class Acceptor;
    class EventLoop;
    class EventLoopThreadPool;

    class TcpServer
    {
    public:
        typedef std::function<void(EventLoop*)> ThreadInitCallback;
        enum Option
        {
            kNoReusePort,
            kReusePort,
        };

        TcpServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const std::string& nameArg,
            Option option = kReusePort);      //TODO: 默认修改成kReusePort
        ~TcpServer();

        const std::string& host_port() const { return host_port_; }
        const std::string& name() const { return name_; }
        EventLoop* get_loop() const { return loop_; }
        ;
        void set_thread_init_callback(const ThreadInitCallback& cb)
        {
            thread_init_callback_ = cb;
        }

        void start(int workerThreadCount = 4);

        void stop();

        /// Set connection callback.
        /// Not thread safe.
        void set_conn_callback(const ConnectionCallback& cb)
        {
            conn_callback_ = cb;
        }

        /// Set message callback.
        /// Not thread safe.
        void set_msg_callback(const MessageCallback& cb)
        {
            msg_callback_ = cb;
        }

        /// Set write complete callback.
        /// Not thread safe.
        void set_write_complete_callback(const WriteCompleteCallback& cb)
        {
            write_complete_callback_ = cb;
        }

        void remove_conn(const TcpConnectionPtr& conn);

    private:
        /// Not thread safe, but in loop
        void new_conn(int sockfd, const InetAddress& peerAddr);
        /// Thread safe.

        /// Not thread safe, but in loop
        void remove_conn_in_loop(const TcpConnectionPtr& conn);

        typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

    private:
        EventLoop* loop_;
        const std::string                                    host_port_;
        const std::string                                    name_;
        std::unique_ptr<Acceptor>                       acceptor_;
        std::unique_ptr<EventLoopThreadPool>            event_loop_threadpool_;
        ConnectionCallback                              conn_callback_;
        MessageCallback                                 msg_callback_;
        WriteCompleteCallback                           write_complete_callback_;
        ThreadInitCallback                              thread_init_callback_;
        std::atomic<int>                                started_;
        int                                             next_conn_id_;
        ConnectionMap                                   conn_map_;
    };

}