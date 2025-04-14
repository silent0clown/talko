#include "http_server.h"
#include "http_session.h"

bool HttpServer::init(const char* ip, short port, EventLoop* loop)
{
    InetAddress addr(ip, port);
    server_.reset(new TcpServer(loop, addr, "WHISP_HTTP_SERVER", TcpServer::kReusePort));
    server_->start();

    return true;
}

void HttpServer::uninit()
{
    if (server_) {
        server_->stop();
    }
}

void HttpServer::on_connected(std::shared_ptr<TcpConnection>conn)
{
    if (conn->connected()) {
        std::shared_ptr<HttpSession> session_sp(new HttpSession(conn));
        conn->set_msg_callback(std::bind(&HttpSession::on_read, session_sp.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        {
            std::lock_guard<std::mutex> guard(session_mutex_);
            sessions_.push_back(session_sp);
        }
    } else {
        on_disconnected(conn);
    }
}