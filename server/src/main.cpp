#include "config/parse_config.h"
#include "db_mysqlconn_pool.h"
#include "log/whisp_log.h"
#include "util/daemon_run.h"
#include <iostream>
// #include <memory>
// #include <stdlib>
#include <csignal>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>    // 文件夹操作句柄
// // #include "service/TalkServer.h"


#define MAX_LOG_FILE_PATH  (256)

// class ServerInitializer {
// public:
//     // 获取单例实例
//     static ServerInitializer& getInstance() {
//         static ServerInitializer instance;
//         return instance;
//     }

//     // 初始化服务器
//     void initialize() {
//         std::cout << "[ServerInitializer] 开始初始化服务器..." << std::endl;

//         // // 1. 加载配置文件
//         // if (!config.loadConfig("config/config.yaml")) {
//         //     std::cerr << "[ERROR] 加载配置失败!" << std::endl;
//         //     exit(EXIT_FAILURE);
//         // }
//         // std::cout << "[ServerInitializer] 配置加载完成" << std::endl;

//         // 2. 初始化日志
//         TalkLog::getInstance().setLogFile("/home/ubuntu/project/talko/server/data/log/talklog.log");
//         TalkLog::getInstance().setLogLevel(LogLevel::INFO);
//         TALKO_LOG_INFO("[ServerInitializer] 日志系统初始化完成");

//         // 3. 连接数据库
//         MySQLConnectionPool::GetInstance()->Init("127.0.0.1", "root", "talko_root", "talko_server", 6603, 5);
//         // std::cout << "[ServerInitializer] 数据库连接成功" << std::endl;

//         initialized = true;
//         std::cout << "[ServerInitializer] 服务器初始化完成" << std::endl;
//     }

// private:
//     // 私有构造函数，防止外部实例化
//     ServerInitializer() : initialized(false) {}

//     // 禁止拷贝构造和赋值
//     ServerInitializer(const ServerInitializer&) = delete;
//     ServerInitializer& operator=(const ServerInitializer&) = delete;

//     // 成员变量
//     bool initialized;
//     // TalkConfig config;
//     // TalkLog log;
//     // TalkDB db;
// };


/*
在 main 函数中，需要完成以下操作：

    初始化日志系统。

    解析命令行参数。

    初始化线程池。

    初始化网络层。

    初始化数据库连接池。

    注册信号处理函数。

    实现主循环。

    清理资源。
    
*/



// 静态成员初始化
// WhispDB* WhispDB::instance_ = nullptr;
// std::mutex WhispDB::mutex_;

// 参数解析
void parse_arguments(int argc, char* argv[])
{
    int ch;
    bool bdaemon = false;
    while ((ch = getopt(argc, argv, "d")) != -1)
    {
        switch (ch)
        {
        case 'd':
            bdaemon = true;
            break;
        }
    }

    if (bdaemon) {
        std::cout << "server run in daemon mode." << std::endl;
        daemon_run();

    }
}

int main(int argc, char* argv[])
{
    std::cout << "[Main] 服务器启动中..." << std::endl;
    char log_file[MAX_LOG_FILE_PATH + 1] = {0};
#ifndef WIND32
    signal(SIGCHLD, SIG_DFL);   // 子进程退出时，默认处理（避免产生僵尸进程）
    signal(SIGPIPE, SIG_IGN);   // 忽略 SIGPIPE 信号（通常在管道破裂、socket 关闭时触发）
    signal(SIGINT, prog_exit);  // SIGINT (Ctrl+C) 和 SIGTERM (终止信号) 时调用 prog_exit 进行清理操作
    signal(SIGTERM, prog_exit);
#endif
    // 参数解析
    parse_arguments(argc, argv);

    // 加载配置
    ConfigParser parser;
    struct WhispConfig whisp_config;
    if (parser.whisp_load_config("/home/dev1/talko/server/src/config/server_config.yml", whisp_config)) {
        parser.whisp_print_config(whisp_config);
    } else {
        std::cerr << "Failed to load config.yml!" << std::endl;
    }

    // 初始化日志， 后续封装实现
    if (whisp_config.log_config.log_file_dir.size() == 0) {
        const char* default_path = "/var/log/whisplog";
        memcpy(log_file, default_path, strlen(default_path));
    } else {
        memcpy(log_file, whisp_config.log_config.log_file_dir.c_str(), strlen(whisp_config.log_config.log_file_dir.c_str()));
    }
    std::cout << "log file path is : " << log_file << std::endl;

    DIR* dp = opendir(log_file);
    if (dp == NULL)
    {
        if (mkdir(log_file, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
        {
        std::cerr << "create base dir error, "<< log_file << ", errno: "<< errno << strerror(errno);
        return 1;
        }
    }
    closedir(dp);
 
//      log_file = log_file_path;

 
//     log_file_name = whisp_config.log_config.log_file_name;
//     if (nullptr = log_file_name) {
//         log_file_name = "whisplog";
//     }
//     log_file += log_file_name;
//     std::cout << "log file is : " << log_file << std:endl; 
 
//  #ifdef _DEBUG
//      CAsyncLog::init();
//  #else
//      CAsyncLog::init(logFileFullPath.c_str());
//  #endif

    // WhispDB* db = WhispDB::get_instance();
    // db->init("localhost", "user", "password", "dbname");

    // // 1. 初始化服务器
    // ServerInitializer::getInstance().initialize();

    // 2. 启动聊天服务器
    // TalkServer server;
    // server.start();

    return 0;
}
