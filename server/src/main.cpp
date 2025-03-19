#include <iostream>
#include <memory>
#include "config/TalkConfig.h"
#include "MySQLConnectionPool.h"
#include "log/TalkLog.h"
#include "service/TalkServer.h"

class ServerInitializer {
public:
    // 获取单例实例
    static ServerInitializer& getInstance() {
        static ServerInitializer instance;
        return instance;
    }

    // 初始化服务器
    void initialize() {
        std::cout << "[ServerInitializer] 开始初始化服务器..." << std::endl;

        // // 1. 加载配置文件
        // if (!config.loadConfig("config/config.yaml")) {
        //     std::cerr << "[ERROR] 加载配置失败!" << std::endl;
        //     exit(EXIT_FAILURE);
        // }
        // std::cout << "[ServerInitializer] 配置加载完成" << std::endl;

        // 2. 初始化日志
        TalkLog::getInstance().setLogFile("/home/ubuntu/project/talko/server/data/log/talklog.log");
        TalkLog::getInstance().setLogLevel(LogLevel::INFO);
        TALKO_LOG_INFO("[ServerInitializer] 日志系统初始化完成");

        // 3. 连接数据库
        MySQLConnectionPool::GetInstance()->Init("127.0.0.1", "root", "talko_root", "talko_server", 6603, 5);
        // std::cout << "[ServerInitializer] 数据库连接成功" << std::endl;

        initialized = true;
        std::cout << "[ServerInitializer] 服务器初始化完成" << std::endl;
    }

private:
    // 私有构造函数，防止外部实例化
    ServerInitializer() : initialized(false) {}

    // 禁止拷贝构造和赋值
    ServerInitializer(const ServerInitializer&) = delete;
    ServerInitializer& operator=(const ServerInitializer&) = delete;

    // 成员变量
    bool initialized;
    // TalkConfig config;
    // TalkLog log;
    // TalkDB db;
};

int main() {
    std::cout << "[Main] 服务器启动中..." << std::endl;
    
    // // 初始化连接池
    // MySQLConnectionPool::GetInstance()->Init("127.0.0.1", "root", "talko_root", "talko_server", 6033, 10);

    // 1. 初始化服务器
    ServerInitializer::getInstance().initialize();

    // 2. 启动聊天服务器
    // TalkServer server;
    // server.start();

    return 0;
}
