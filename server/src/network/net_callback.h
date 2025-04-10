#pragma once

#include <functional>
#include <memory>
#include "../common/whisp_timestamp.h"
#include "log/whisp_log.h"




namespace w_network
{
    class ByteBuffer;
    class TcpConnection;
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
    typedef std::function<void()> TimerCallback;
    typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;
    typedef std::function<void(const TcpConnectionPtr&)> CloseCallback;
    typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
    typedef std::function<void(const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

    typedef std::function<void(const TcpConnectionPtr&, ByteBuffer*, Timestamp)> MessageCallback;
    void default_conn_callback(const TcpConnectionPtr& conn);
    void default_msg_callback(const TcpConnectionPtr&, ByteBuffer* buffer, Timestamp);
}

// void network::default_conn_callback(const TcpConnectionPtr& conn)
// {
//     LOGD("%s -> is %s",
//         conn->localAddress().toIpPort().c_str(),
//         conn->peerAddress().toIpPort().c_str(),
//         (conn->connected() ? "UP" : "DOWN"));
//     // do not call conn->forceClose(), because some users want to register message callback only.
// }

// void network::default_msg_callback(const TcpConnectionPtr&, ByteBuffer* buffer, Timestamp)
// {
//     buffer->retrieveAll();
// }