#include "tcp_connect.h"
#include "log/whisp_log.h"

using namespace network;

void network::default_conn_callback(const TcpConnectionPtr& conn) 
{
    WHISP_LOG_DEBUG("%s -> is %s",
        conn->localAddress().toIpPort().c_str(),
        conn->peerAddress().toIpPort().c_str(),
        (conn->connected() ? "UP" : "DOWN"));
}

void network::default_msg_callback(const TcpConnectionPtr& conn, ByteBuffer* buffer, Timestamp recv_time) 
{
    buffer->retrieveAll();
}