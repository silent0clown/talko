#ifndef PARSE_CONFIG_H
#define PARSE_CONFIG_H

#include <string>
#include <yaml-cpp/yaml.h>

struct ClientConfig {
    std::string client_listen_ip;
    int client_listen_port;
};

struct MonitorConfig {
    std::string monitor_listen_ip;
    int monitor_listen_port;
    std::string monitor_token;
};

struct HttpConfig {
    std::string http_listen_ip;
    int http_listen_port;
};

struct LogConfig {
    std::string log_file_dir;
    std::string log_file_name;
    bool log_binary_package;
};

struct MysqlConfig {
    std::string mysql_server_addr;
    int mysql_server_port;
    std::string user;
    std::string password;
    std::string database;
};

struct WhispConfig {
    struct ClientConfig client_config;
    struct MonitorConfig monitor_config;
    struct HttpConfig http_config;
    struct LogConfig log_config;
    struct MysqlConfig mysql_config;
};

class ConfigParser {
public:
    // 入参1：配置文件路径， // 入参2：待加载配置项指针
    bool whisp_load_config(const std::string& file_name, struct WhispConfig& whisp_config);
    // bool whisp_destroy_config(struct WhispConfig** whisp_config);
// private:

    // ClientConfig clientConfig;
    // MonitorConfig monitorConfig;
    // HttpConfig httpConfig;
    // LogConfig logConfig;
    // MysqlConfig mysqlConfig;
};


#endif // PARSE_CONFIG_H