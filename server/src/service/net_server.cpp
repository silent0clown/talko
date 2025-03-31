#include "whisp_net_server.h"
#include "whisp_log.h"  // 引入日志头文件
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace net_server {

// 构造函数，初始化服务器
Server::Server(int port) 
    : server_fd_(-1), epoll_fd_(-1), running_(false) {
    // 创建监听套接字
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        WHISP_LOG_ERROR("Socket creation failed!");
        return;
    }

    struct sockaddr_in server_addr {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // 绑定套接字
    if (bind(server_fd_, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        WHISP_LOG_ERROR("Bind failed!");
        close(server_fd_);
        return;
    }

    // 设置为监听状态
    if (listen(server_fd_, 10) == -1) {
        WHISP_LOG_ERROR("Listen failed!");
        close(server_fd_);
        return;
    }

    // 创建epoll文件描述符
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        WHISP_LOG_ERROR("Epoll create failed!");
        close(server_fd_);
        return;
    }

    // 将监听套接字加入epoll监控
    struct epoll_event event {};
    event.events = EPOLLIN;
    event.data.fd = server_fd_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_fd_, &event) == -1) {
        WHISP_LOG_ERROR("Epoll ctl failed!");
        close(server_fd_);
        close(epoll_fd_);
    } else {
        WHISP_LOG_INFO("Server initialized and listening on port " + std::to_string(port));
    }
}

// 析构函数，清理资源
Server::~Server() {
    stop();
    if (server_fd_ != -1) {
        close(server_fd_);
        WHISP_LOG_INFO("Server socket closed.");
    }
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
        WHISP_LOG_INFO("Epoll file descriptor closed.");
    }
}

// 启动服务器
void Server::start() {
    running_ = true;
    WHISP_LOG_INFO("Server started, awaiting events...");
    while (running_) {
        handle_events();
    }
}

// 停止服务器
void Server::stop() {
    running_ = false;
    WHISP_LOG_INFO("Server stopped.");
    for (auto& worker_thread : worker_threads_) {
        if (worker_thread.joinable()) {
            worker_thread.join();
        }
    }
}

// 处理事件
void Server::handle_events() {
    struct epoll_event events[max_events_];
    int num_events = epoll_wait(epoll_fd_, events, max_events_, -1);

    if (num_events == -1) {
        WHISP_LOG_ERROR("Epoll wait failed!");
        return;
    }

    for (int i = 0; i < num_events; ++i) {
        if (events[i].data.fd == server_fd_) {
            // 新连接到来，接受连接
            struct sockaddr_in client_addr {};
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd_, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
            if (client_fd == -1) {
                WHISP_LOG_ERROR("Accept failed!");
                continue;
            }

            // 将客户端套接字加入epoll监控
            struct epoll_event event {};
            event.events = EPOLLIN;
            event.data.fd = client_fd;
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                WHISP_LOG_ERROR("Epoll ctl failed for client!");
                close(client_fd);
            } else {
                WHISP_LOG_INFO("New client connected.");
            }
        } else {
            // 处理客户端消息
            process_message(events[i].data.fd);
        }
    }
}

// 处理客户端发送的消息
void Server::process_message(int client_fd) {
    char buffer[1024];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));

    if (bytes_read <= 0) {
        // 客户端关闭连接或读取失败
        WHISP_LOG_WARN("Client " + std::to_string(client_fd) + " disconnected.");
        close(client_fd);
    } else {
        // 处理消息 (可以根据消息类型进行分发)
        WHISP_LOG_DEBUG("Received message: " + std::string(buffer, bytes_read));
    }
}



}  // namespace whisp_net_server


