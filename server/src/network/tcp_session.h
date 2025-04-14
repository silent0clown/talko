
#pragma once

#include "tcp_connect.h"
#include <memory>

using namespace w_network;

class TcpSession
{
public:
    TcpSession(const std::weak_ptr<TcpConnection>& tmpconn);
    ~TcpSession();

    TcpSession(const TcpSession& rhs) = delete;
    TcpSession& operator =(const TcpSession& rhs) = delete;

    std::shared_ptr<TcpConnection> get_conn_ptr()
    {
        if (tmp_conn_.expired())
            return NULL;

        return tmp_conn_.lock();
    }

    void send(int32_t cmd, int32_t seq, const std::string& data);
    void send(int32_t cmd, int32_t seq, const char* data, int32_t dataLength);
    void send(const std::string& p);
    void send(const char* p, int32_t length);

private:
    void send_pkg(const char* p, int32_t length);

protected:
    std::weak_ptr<TcpConnection>    tmp_conn_;
};
