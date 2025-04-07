// whisp_mysqlconn_pool.cpp
#include "whisp_mysqlconn_pool.h"
#include "whisp_log.h"
#include <iostream>

// 实现纯虚析构函数
WhispSqlConn::~WhispSqlConn() {
    // 纯虚析构函数的定义
}

// MysqlConn类实现
MysqlConn::MysqlConn() : session(nullptr) {}

MysqlConn::~MysqlConn() {
    if (session) {
        delete session;
    }
}

bool MysqlConn::connect(const std::string& host, const int port, const std::string& user, const std::string& password, const std::string& database) {
    try {
        session = new mysqlx::Session(host, port, user, password, database);
        WHISP_LOG_INFO("[MySQL] Create mysql connect success");
        return true;
    } catch (const mysqlx::Error& e) {
        // std::cout << "MySQL connection error: " << e.what() << std::endl;
        WHISP_LOG_ERROR("MySQL connection error: %s", e.what());
        return false;
    }
}

void MysqlConn::disconnect() {
    if (session) {
        delete session;
        session = nullptr;
    }
}

bool MysqlConn::execute(const std::string& sql) {
    if (!session) {
        return false;
    }
    try {
        session->sql(sql).execute();
        return true;
    } catch (const mysqlx::Error& e) {
        WHISP_LOG_ERROR("MySQL execute error: ", e.what());
        return false;
    }
}

// MysqlConnPool 类实现
MysqlConnPool::MysqlConnPool(const std::string& host, const int port, const std::string& user, const std::string& password, const std::string& database, size_t pool_size)
    : host_(host), user_(user), password_(password), database_(database), pool_size_(pool_size) {
    for (size_t i = 0; i < pool_size; ++i) {
        auto conn = std::make_shared<MysqlConn>();
        if (conn->connect(host,port, user, password, database)) {
            conn_queue_.push(conn);
        } else {
            WHISP_LOG_ERROR("Failed to create a connection for the pool.");
        }
    }
}

MysqlConnPool::~MysqlConnPool() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!conn_queue_.empty()) {
        auto conn = conn_queue_.front();
        conn->disconnect();
        conn_queue_.pop();
    }
}

std::shared_ptr<MysqlConn> MysqlConnPool::get_conn(int timeout_ms) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() { return !conn_queue_.empty(); })) {
        return nullptr; // 超时返回空指针
    }
    // cond_.wait(lock, [this] { return!conn_queue_.empty(); });
    auto conn = conn_queue_.front();
    conn_queue_.pop();
    return conn;
}

void MysqlConnPool::release_conn(std::shared_ptr<MysqlConn> conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    conn_queue_.push(conn);
    cond_.notify_one();
}



bool MysqlConnPool::_check_db_exist() {
    std::shared_ptr<MysqlConn> conn = get_conn(); // 从连接池获取连接
    std::string query = "SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = '" + database_ + "'";

    try {
        // 执行查询
        auto result = conn->execute(query);
        return result;  // 如果查询结果为true，则数据库存在
    } catch (const std::exception& e) {
        std::cerr << "Error checking database existence: " << e.what() << std::endl;
        return false;
    }
}

bool MysqlConnPool::_create_db() {
    std::shared_ptr<MysqlConn> conn = get_conn(); // 从连接池获取连接
    std::string query = "CREATE DATABASE " + database_;

    try {
        // 执行创建数据库命令
        conn->execute(query);
        return true; // 如果执行成功，返回 true
    } catch (const std::exception& e) {
        std::cerr << "Error creating database: " << e.what() << std::endl;
        return false;
    }
}

bool MysqlConnPool::_check_table_exist(const TableInfo& table) {
    std::shared_ptr<MysqlConn> conn = get_conn(); // 从连接池获取连接
    std::string query = "SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = '" + database_ + "' AND TABLE_NAME = '" + table.name_ + "'";

    try {
        // 执行查询
        auto result = conn->execute(query);
        return result;  // 如果查询结果非空，则表存在
    } catch (const std::exception& e) {
        std::cerr << "Error checking table existence: " << e.what() << std::endl;
        return false;
    }
}

bool MysqlConnPool::_create_table(const TableInfo& table) {
    std::shared_ptr<MysqlConn> conn = get_conn(); // 从连接池获取连接
    std::string query = "CREATE TABLE " + table.name_ + " (";

    // 遍历表字段，并构造字段的 SQL 定义
    for (const auto& field : table.field_) {
        query += field.second.name_ + " " + field.second.type_ + " " + field.second.desc_+ ", ";
    }

    // 移除末尾的逗号和空格
    query = query.substr(0, query.size() - 2);
    query += ") " + table.key_;  // 添加主键和索引

    try {
        // 执行创建表命令
        conn->execute(query);
        return true; // 如果执行成功，返回 true
    } catch (const std::exception& e) {
        std::cerr << "Error creating table: " << e.what() << std::endl;
        return false;
    }
}

bool MysqlConnPool::_update_table(const TableInfo& table) {
    std::shared_ptr<MysqlConn> conn = get_conn(); // 从连接池获取连接

    // 这里假设你要增加新的字段或修改表
    for (const auto& field : table.field_) {
        std::string query = "ALTER TABLE " + table.name_ + " ADD COLUMN " + field.second.name_+ " " + field.second.type_ + " " + field.second.desc_;

        try {
            // 执行更新表结构的命令
            conn->execute(query);
        } catch (const std::exception& e) {
            std::cerr << "Error updating table: " << e.what() << std::endl;
            return false;
        }
    }

    return true;
}


// WhispSqlConnPool::WhispSqlConnPool(const std::string& host, const std::string& user,
//                                    const std::string& password, const std::string& db,
//                                    int port, int max_conn)
//     : host_(host), user_(user), password_(password), db_(db), port_(port), max_conn_(max_conn) {
//     for (int i = 0; i < max_conn_; ++i) {
//         Session* conn = createConnection();
//         if (conn) {
//             conn_queue_.push(conn);
//             ++conn_count_;
//         }
//     }
// }

// WhispSqlConnPool::~WhispSqlConnPool() {
//     std::lock_guard<std::mutex> lock(mutex_);
//     while (!conn_queue_.empty()) {
//         Session* conn = conn_queue_.front();
//         conn_queue_.pop();
//         delete conn;
//     }
//     conn_count_ = 0;
// }

// Session* WhispSqlConnPool::createConnection() {
//     try {
//         SessionSettings settings(host_, port_, user_, password_, db_);
//         return new Session(settings);
//     } catch (const mysqlx::Error& err) {
//         std::cerr << "[MySQLX] Connection creation failed: " << err.what() << std::endl;
//         return nullptr;
//     }
// }

// WhispSqlConnPtr WhispSqlConnPool::getConnection() {
//     std::unique_lock<std::mutex> lock(mutex_);
//     cond_.wait(lock, [this]() { return !conn_queue_.empty(); });

//     Session* conn = conn_queue_.front();
//     conn_queue_.pop();
//     return WhispSqlConnPtr(conn);
// }

// void WhispSqlConnPool::releaseConnection(Session* conn) {
//     if (conn) {
//         std::lock_guard<std::mutex> lock(mutex_);
//         conn_queue_.push(conn);
//         cond_.notify_one();
//     }
// }

// WhispSqlConnFactory& WhispSqlConnFactory::getInstance() {
//     static WhispSqlConnFactory instance;
//     return instance;
// }

// void WhispSqlConnFactory::createPool(const std::string& name,
//                                      const std::string& host, const std::string& user,
//                                      const std::string& password, const std::string& db,
//                                      int port, int max_conn) {
//     std::lock_guard<std::mutex> lock(mutex_);
//     if (pool_map_.find(name) == pool_map_.end()) {
//         pool_map_[name] = std::make_shared<WhispSqlConnPool>(host, user, password, db, port, max_conn);
//     }
// }

// std::shared_ptr<WhispSqlConnPool> WhispSqlConnFactory::getPool(const std::string& name) {
//     std::lock_guard<std::mutex> lock(mutex_);
//     auto it = pool_map_.find(name);
//     if (it != pool_map_.end()) {
//         return it->second;
//     }
//     return nullptr;
// }
