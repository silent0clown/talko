#include "App.h"
#include <iostream>
#include <string>

// 定义自定义数据结构
struct PerSocketData {
    int userId;
    std::string username;
};

// 测试客户端连接到服务器
int main() {
    std::string url = "ws://127.0.0.1:3000"; // WebSocket 服务器地址

    uWS::App().ws<PerSocketData>("/*", {
        .open = [](auto* ws) {
            std::cout << "[INFO] Connected to WebSocket server!" << std::endl;

            // 初始化用户数据
            PerSocketData* data = ws->getUserData();
            data->userId = 123;
            data->username = "Guest";

            // 发送欢迎消息
            std::string message = "Hello from " + data->username + " (ID: " + std::to_string(data->userId) + ")";
            uWS::OpCode msgType = uWS::OpCode::TEXT;

            if (ws->send(message, msgType)) {
                std::cout << "[SEND] UserID: " << data->userId << " | Username: " << data->username
                          << " | Message: " << message
                          << " | Type: " << (msgType == uWS::OpCode::TEXT ? "TEXT" : "BINARY") << std::endl;
            } else {
                std::cerr << "[ERROR] Failed to send message!" << std::endl;
            }
        },
        .message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {
            PerSocketData* data = ws->getUserData();
            std::cout << "[RECEIVE] UserID: " << data->userId << " | Username: " << data->username
                      << " | Message: " << message
                      << " | Type: " << (opCode == uWS::OpCode::TEXT ? "TEXT" : "BINARY") << std::endl;
        },
        .close = [](auto* ws, int code, std::string_view message) {
            PerSocketData* data = ws->getUserData();
            std::cout << "[INFO] UserID: " << data->userId << " | Username: " << data->username
                      << " | Disconnected! Code: " << code << ", Reason: " << message << std::endl;
        }
    }).connect(url, nullptr).run(); // 连接 WebSocket 并运行事件循环

    return 0;
}
