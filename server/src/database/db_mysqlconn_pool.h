#ifndef MYSQL_CONNECTION_POOL_H
#define MYSQL_CONNECTION_POOL_H

#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <mysqlx/xdevapi.h>
#include "whisp_log.h"

class MySQLConnectionPool {
public:
    // 获取单例对象
    static MySQLConnectionPool* GetInstance() {
        static MySQLConnectionPool instance;
        return &instance;
    }

    // 初始化连接池
    void Init(const std::string& host, const std::string& user, const std::string& passwd,
              const std::string& dbname, int port, int pool_size) {
        std::lock_guard<std::mutex> lock(mutex_);
        this->host_ = host;
        this->user_ = user;
        this->passwd_ = passwd;
        this->dbname_ = dbname;
        this->port_ = port;
        this->pool_size_ = pool_size;

        int success_count = 0;
        // for (int i = 0; i < pool_size_; ++i) {
        TALKO_LOG_DEBUG(" Enter MySQLConnectPool init ");
        while (conn_pool_.size() < pool_size_ ) {
            try {
                mysqlx::Session session(host, port, user, passwd, dbname);
                conn_pool_.push(std::make_unique<mysqlx::Session>(std::move(session)));
                success_count++;
            } catch (const mysqlx::Error& err) {
                TALKO_LOG_ERROR("MySQL 连接失败: ", err.what());
            }
        }
        if (success_count == 0 && conn_pool_.size() == pool_size_) {
            TALKO_LOG_WARN("pool size is full");
            return;
        }

        if (success_count == 0) {
            TALKO_LOG_ERROR("无法初始化任何 MySQL 连接");
            throw std::runtime_error("无法初始化任何 MySQL 连接");
        }
        TALKO_LOG_INFO("初始化连接池成功，连接数： ", success_count);
    }

    /*
        try {
            auto conn = MySQLConnectionPool::GetInstance()->GetConnection();
            if (conn) {
                // 使用连接
            }
        } catch (const std::exception& e) {
            std::cerr << "获取连接失败: " << e.what() << std::endl;
        }
    */
    // 获取连接
    std::unique_ptr<mysqlx::Session> GetConnection(int timeout_ms = 5000) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() { return !conn_pool_.empty(); })) {
            throw std::runtime_error("获取连接超时");
        }
    
        std::unique_ptr<mysqlx::Session> conn = std::move(conn_pool_.front());
        conn_pool_.pop();
        // fixit: conn的连接健康检查
        // std::cout << "pool delete, cur size : " <<conn_pool_.size() <<  std::endl;
        return conn;
    }

    // 归还连接
    void ReleaseConnection(std::unique_ptr<mysqlx::Session> conn) {
        if (!conn) {    // 如果 conn 是空指针，就创建了一个新的 mysqlx::Session 连接，并放回 conn_pool_
            try { 
                conn = std::make_unique<mysqlx::Session>(host_, port_, user_, passwd_, dbname_);
                std::cout << "function in it" << std::endl;
            } catch (const mysqlx::Error& err) {
                std::cerr << "MySQL 连接失败: " << err.what() << std::endl;
                return;
            }
        }
    
        std::lock_guard<std::mutex> lock(mutex_);
        conn_pool_.push(std::move(conn));
        // std::cout << "pool add, cur size: " << conn_pool_.size() <<  std::endl;
        cond_.notify_one();
    }

    // 添加公共方法获取连接池大小
    size_t GetConnectionPoolSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return conn_pool_.size();
    }
    
    // 释放所有连接
    ~MySQLConnectionPool() {
        while (!conn_pool_.empty()) {
            auto conn = std::move(conn_pool_.front());
            if (conn) {
                conn->close();
            }
            conn_pool_.pop();
        }
    }

private:
    MySQLConnectionPool() = default;
    MySQLConnectionPool(const MySQLConnectionPool&) = delete;
    MySQLConnectionPool& operator=(const MySQLConnectionPool&) = delete;

    std::queue<std::unique_ptr<mysqlx::Session>> conn_pool_;    // 连接数
    mutable std::mutex mutex_;
    std::condition_variable cond_;

    std::string host_, user_, passwd_, dbname_;
    int port_, pool_size_;  // pool_size_ 池大小
};    

#endif // MYSQL_CONNECTION_POOL_H
