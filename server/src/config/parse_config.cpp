#include "parse_config.h"
#include <iostream>

bool ConfigParser::whisp_load_config(const std::string &file_name, struct WhispConfig &whisp_config)
{
    try {
        YAML::Node config = YAML::LoadFile(file_name);

        // 解析 Client 配置
        if (config["client"]) {
            if (config["client"]["listen_ip"].IsDefined())
                whisp_config.client_config.client_listen_ip = config["client"]["listen_ip"].as<std::string>();
            if (config["client"]["listen_port"].IsDefined())
                whisp_config.client_config.client_listen_port = config["client"]["listen_port"].as<int>();
        }

        // 解析 Monitor 配置
        if (config["monitor"]) {
            if (config["monitor"]["listen_ip"].IsDefined())
                whisp_config.monitor_config.monitor_listen_ip = config["monitor"]["listen_ip"].as<std::string>();
            if (config["monitor"]["listen_port"].IsDefined())
                whisp_config.monitor_config.monitor_listen_port = config["monitor"]["listen_port"].as<int>();
            if (config["monitor"]["token"].IsDefined())
                whisp_config.monitor_config.monitor_token = config["monitor"]["token"].as<std::string>();
        }

        // 解析 HTTP 配置
        if (config["http"]) {
            if (config["http"]["listen_ip"].IsDefined())
                whisp_config.http_config.http_listen_ip = config["http"]["listen_ip"].as<std::string>();
            if (config["http"]["listen_port"].IsDefined())
                whisp_config.http_config.http_listen_port = config["http"]["listen_port"].as<int>();
        }

        // 解析 Log 配置
        if (config["log"]) {
            if (config["log"]["file_dir"].IsDefined())
                whisp_config.log_config.log_file_dir = config["log"]["file_dir"].as<std::string>();
            if (config["log"]["file_name"].IsDefined())
                whisp_config.log_config.log_file_name = config["log"]["file_name"].as<std::string>();
            if (config["log"]["binary_package"].IsDefined())
                whisp_config.log_config.log_binary_package = config["log"]["binary_package"].as<bool>();
        }

        // 解析 MySQL 配置
        if (config["mysql"]) {
            if (config["mysql"]["server_ip"].IsDefined())
                whisp_config.mysql_config.mysql_server_addr = config["mysql"]["server_ip"].as<std::string>();
            if (config["mysql"]["server_port"].IsDefined())
                whisp_config.mysql_config.mysql_server_port = std::stoi(config["mysql"]["server_port"].as<std::string>());
            if (config["mysql"]["user"].IsDefined())
                whisp_config.mysql_config.user = config["mysql"]["user"].as<std::string>();
            if (config["mysql"]["password"].IsDefined())
                whisp_config.mysql_config.password = config["mysql"]["password"].as<std::string>();
            if (config["mysql"]["database"].IsDefined())
                whisp_config.mysql_config.database = config["mysql"]["database"].as<std::string>();
        }

        return true;

    } catch (const YAML::Exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        return false;
    }
}

void ConfigParser::whisp_print_config(const struct WhispConfig& whisp_config) {
    // 打印 Client 配置
    std::cout << "Client Config:" << std::endl;
    std::cout << "  Listen IP: " << whisp_config.client_config.client_listen_ip << std::endl;
    std::cout << "  Listen Port: " << whisp_config.client_config.client_listen_port << std::endl;

    // 打印 Monitor 配置
    std::cout << "Monitor Config:" << std::endl;
    std::cout << "  Listen IP: " << whisp_config.monitor_config.monitor_listen_ip << std::endl;
    std::cout << "  Listen Port: " << whisp_config.monitor_config.monitor_listen_port << std::endl;
    std::cout << "  Token: " << whisp_config.monitor_config.monitor_token << std::endl;

    // 打印 HTTP 配置
    std::cout << "HTTP Config:" << std::endl;
    std::cout << "  Listen IP: " << whisp_config.http_config.http_listen_ip << std::endl;
    std::cout << "  Listen Port: " << whisp_config.http_config.http_listen_port << std::endl;

    // 打印 Log 配置
    std::cout << "Log Config:" << std::endl;
    std::cout << "  File Directory: " << whisp_config.log_config.log_file_dir << std::endl;
    std::cout << "  File Name: " << whisp_config.log_config.log_file_name << std::endl;
    std::cout << "  Binary Package: " << (whisp_config.log_config.log_binary_package ? "true" : "false") << std::endl;

    // 打印 MySQL 配置
    std::cout << "MySQL Config:" << std::endl;
    std::cout << "  Server IP: " << whisp_config.mysql_config.mysql_server_addr << std::endl;
    std::cout << "  Server Port: " << whisp_config.mysql_config.mysql_server_port << std::endl;
    std::cout << "  User: " << whisp_config.mysql_config.user << std::endl;
    std::cout << "  Password: " << whisp_config.mysql_config.password << std::endl;
    std::cout << "  Database: " << whisp_config.mysql_config.database << std::endl;
}
