#pragma once

// #include <memory>
#include "net_callback.h"
#include "inet_address.h"
#include "byte_buffer.h"

// struct tcp_info;  // ?
struct tcp_info;

namespace w_network
{
    class EventLoop;
    class Channel;
    class Socket;

    class TcpConnection : public std::enable_shared_from_this<TcpConnection>
    {
    public:
        TcpConnection(EventLoop* loop,
            const std::string& name,
            int sockfd,
            const InetAddress& localAddr,
            const InetAddress& peerAddr);
        ~TcpConnection();

        EventLoop* get_loop() const { return loop_; }
        const std::string& name() const { return name_; }
        const InetAddress& local_address() const { return local_addr_; }
        const InetAddress& peer_address() const { return peer_addr_; }
        bool connected() const { return state_ == kConnected; }

        void send(const void* message, int len);
        void send(const std::string& message);
        void send(ByteBuffer* message);  // this one will swap data
        void shutdown();
        void force_close();

        void set_tcp_nodelay(bool on);

        void set_conn_callback(const ConnectionCallback& cb)
        {
            conn_callback_ = cb;
        }

        void set_msg_callback(const MessageCallback& cb)
        {
            msg_callback_ = cb;
        }

        //设置成功发完数据执行的回调
        void set_write_complete_callback(const WriteCompleteCallback& cb)
        {
            write_complete_callback_ = cb;
        }

        void set_high_water_mark_callback(const HighWaterMarkCallback& cb, size_t highWaterMark)
        {
            high_water_mark_callback_ = cb; 
            high_water_mark_ = highWaterMark;
        }

        ByteBuffer* input_buffer()
        {
            return &input_buffer_;
        }

        ByteBuffer* output_buffer()
        {
            return &output_buffer_;
        }

        // Internal use only.
        void set_close_callback(const CloseCallback& cb)
        {
            close_callback_ = cb;
        }

        void conn_established();
        void conn_destroyed();

    private:
        enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
        void _handle_read(Timestamp receiveTime);
        void _handle_write();
        void _handle_close();
        void _handle_error();
        // void sendInLoop(std::string&& message);
        void _send_in_loop(const std::string& message);
        void _send_in_loop(const void* message, size_t len);
        void _shutdown_in_loop();
        // void shutdownAndForceCloseInLoop(double seconds);
        void _force_close_in_loop();
        void _set_state(StateE s) { state_ = s; }
        const char* _state_2_string() const;

    private:
        EventLoop* loop_;
        const std::string           name_;
        StateE                      state_;
        std::unique_ptr<Socket>     socket_;
        std::unique_ptr<Channel>    channel_;
        const InetAddress           local_addr_;
        const InetAddress           peer_addr_;
        ConnectionCallback          conn_callback_;
        MessageCallback             msg_callback_;
        WriteCompleteCallback       write_complete_callback_;
        HighWaterMarkCallback       high_water_mark_callback_;
        CloseCallback               close_callback_;
        size_t                      high_water_mark_;
        ByteBuffer                  input_buffer_;
        ByteBuffer                  output_buffer_;
    };

    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

}

