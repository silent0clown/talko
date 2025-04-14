#pragma once

#include "network/byte_buffer.h"
#include "network/tcp_connect.h"
#include "network/tcp_session.h"
#include "common/whisp_timestamp.h"

using namespace w_network;

class HttpSession {
public:
    HttpSession(std::shared_ptr<TcpConnection>& conn);
    ~HttpSession() = default;
    HttpSession(const HttpSession& rhs) = delete;
    HttpSession& operator =(const HttpSession& rhs) = delete;

public:
    void on_read(const std::shared_ptr<TcpConnection>& conn, ByteBuffer* pBuffer, Timestamp receivTime);

    std::shared_ptr<TcpConnection> get_conn_ptr()
    {
        if (tmp_conn_.expired())
            return NULL;

        return tmp_conn_.lock();
    }

    void send(const char* data, size_t length);

private:
    bool _process(const std::shared_ptr<TcpConnection>& conn, const std::string& url, const std::string& param);
    void _makeup_response(const std::string& input, std::string& output);

    void _on_register_response(const std::string& data, const std::shared_ptr<TcpConnection>& conn);
    void _on_login_response(const std::string& data, const std::shared_ptr<TcpConnection>& conn);
    
private:
    std::weak_ptr<TcpConnection>       tmp_conn_;
};