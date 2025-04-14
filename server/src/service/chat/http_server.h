
#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include <memory>
#include <mutex>
#include <list>
#include "network/event_loop.h"
#include "network/tcp_server.h"

using namespace w_network;

//class EventLoop;
//class TcpConnection;
//class TcpServer;
//class EventLoopThreadPool;

class HttpSession;

class HttpServer final
{
public:
    HttpServer() = default;
    ~HttpServer() = default;

    HttpServer(const HttpServer& rhs) = delete;
    HttpServer& operator =(const HttpServer& rhs) = delete;

public:
    bool init(const char* ip, short port, EventLoop* loop);
    void uninit();


    void on_connected(std::shared_ptr<TcpConnection> conn);

    void on_disconnected(const std::shared_ptr<TcpConnection>& conn);

private:
    std::unique_ptr<TcpServer>                     server_;
    std::list<std::shared_ptr<HttpSession>>        sessions_;
    std::mutex                                     session_mutex_; 
};


#endif //!__HTTP_SERVER_H__