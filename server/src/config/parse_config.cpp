#include "parse_config.h"
#include <iostream>

bool ConfigParser::whisp_load_config(const std::string &file_name, struct WhispConfig &whisp_config)
{
    try {
        YAML::Node config = YAML::LoadFile(file_name);

        // 解析 Client 配置
        if (config["client"]) {
            whisp_config.client_config.client_listen_ip = config["client"]["listen_ip"].as<std::string>();
            whisp_config.client_config.client_listen_port = config["client"]["listen_port"].as<int>();
        }

        // 解析 Monitor 配置
        if (config["monitor"]) {
            whisp_config.monitor_config.monitor_listen_ip = config["monitor"]["listen_ip"].as<std::string>();
            whisp_config.monitor_config.monitor_listen_port = config["monitor"]["listen_port"].as<int>();
            whisp_config.monitor_config.monitor_token = config["monitor"]["token"].as<std::string>();
        }

        // 解析 HTTP 配置
        if (config["http"]) {
            whisp_config.http_config.http_listen_ip = config["http"]["listen_ip"].as<std::string>();
            whisp_config.http_config.http_listen_port = config["http"]["listen_port"].as<int>();
        }

        // 解析 Log 配置
        if (config["log"]) {
            whisp_config.log_config.log_file_dir = config["log"]["file_dir"].as<std::string>();
            whisp_config.log_config.log_file_name = config["log"]["file_name"].as<std::string>();
            whisp_config.log_config.log_binary_package = config["log"]["binary_package"].as<bool>();
        }

        // 解析 MySQL 配置
        if (config["mysql"]) {
            whisp_config.mysql_config.mysql_server_addr = config["mysql"]["server"].as<std::string>();
            whisp_config.mysql_config.mysql_server_port = config["mysql"]["port"].as<int>();
            whisp_config.mysql_config.user = config["mysql"]["user"].as<std::string>();
            whisp_config.mysql_config.password = config["mysql"]["password"].as<std::string>();
            whisp_config.mysql_config.database = config["mysql"]["database"].as<std::string>();
        }

        return true;

    } catch (const YAML::Exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        return false;
    }
}