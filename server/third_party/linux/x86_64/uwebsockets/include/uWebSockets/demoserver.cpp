#include "App.h"
#include <iostream>

// 定义自定义数据结构
struct PerSocketData {
    int userId;
    std::string username;
};

int main() {
    // 创建 WebSocket 服务器
    uWS::App app;

    // 处理 WebSocket 连接事件
    app.ws<PerSocketData>("/*", {
        .open = [](auto* ws) {
            std::cout << "Client connected!" << std::endl;
            // 初始化自定义数据
            ws->getUserData()->userId = 123;
            ws->getUserData()->username = "Guest";
        },
        .message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {
            std::cout << "Received message from user " << ws->getUserData()->userId << ": " << message << std::endl;
            // 将消息回显给客户端
            ws->send(message, opCode);
        },
        .close = [](auto* ws, int code, std::string_view message) {
            std::cout << "Client " << ws->getUserData()->username << " disconnected!" << std::endl;
        }
    });

    // 监听端口 3000
    app.listen("127.0.0.1", 3000, [](auto* listenSocket) {
        if (listenSocket) {
            std::cout << "Server started on port 3000!" << std::endl;
        } else {
            std::cerr << "Failed to start server!" << std::endl;
        }
    });

    // 运行事件循环
    app.run();
}