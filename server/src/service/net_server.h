#ifndef WHISP_NET_SERVER_H
#define WHISP_NET_SERVER_H

/*
* 本文件用于实现服务器处理网络消息
* 架构：reactor模式，通过epoll接收消息，根据消息类型，调用相应函数，分发到线程池中执行
*/
#include <sys/epoll.h>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace whisp_net_server {

class Server {
public:
    Server(int port);
    ~Server();

    void start();
    void stop();

private:
    void handle_events();
    void process_message(int client_fd);
    
    int server_fd_;
    int epoll_fd_;
    bool running_;
    std::vector<std::thread> worker_threads_;
    static const int max_events_ = 10;
};

}  // namespace whisp_net_server

#endif // whisp_net_server.h