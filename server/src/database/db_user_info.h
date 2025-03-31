#ifndef TALK_USER_DAO_H
#define TALK_USER_DAO_H

#include <mysqlx/xdevapi.h>
#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <ctime>
#include "db_mysqlconn_pool.h"
#include "whisp_log.h"

// 假设 UserInfo 结构体定义如下
struct UserInfo {
    int id;
    std::string username;
    std::string password_hash;
    std::string nickname;
    std::string avatar_url;
    std::string gender;  // 'male', 'female', 'other'
    std::string birthday; // Date format: 'YYYY-MM-DD'
    std::string phone_number;
    std::string email;
    std::string region;
    std::string status;  // 'active', 'inactive', 'banned'
    std::string last_login_time;
    std::string create_time;
    std::string update_time;
};

class WhispUserDAO {
public:
    // 构造函数：初始化默认连接参数
    explicit WhispUserDAO(
        mysqlx::Session* default_conn = nullptr,
        std::string default_schema = "",
        std::string default_table = ""
    ) : default_conn_(default_conn),
    default_schema_(std::move(default_schema)),
    default_table_(std::move(default_table)) {}

    // ------- 核心操作 ---------
    // 插入新用户
    bool InsertUser(const UserInfo& user, mysqlx::Session* conn_override = nullptr, const std::string& schema_override = "",
                    const std::string& table_override = "");
    
    std::unique_ptr<UserInfo> GetUserById(int user_id, mysqlx::Session* conn_override = nullptr,
                        const std::string& schema_override = "",
                        const std::string& table_override = "");

    bool UpdateUser(const UserInfo& user,
    mysqlx::Session* conn_override = nullptr,
    const std::string& schema_override = "",
    const std::string& table_override = "");

    bool DeleteUser(int user_id,
    mysqlx::Session* conn_override = nullptr,
    const std::string& schema_override = "",
    const std::string& table_override = "");


private:
    // 获取实际使用的参数（优先用override值）
    inline void ResolveParams(
        mysqlx::Session** conn,
        std::string* schema,
        std::string* table,
        mysqlx::Session* conn_override,
        const std::string& schema_override,
        const std::string& table_override) const
    {
        *conn = conn_override ? conn_override : default_conn_;
        *schema = !schema_override.empty() ? schema_override : default_schema_;
        *table = !table_override.empty() ? table_override : default_table_;
    }

    UserInfo ParseUser(const mysqlx::Row& row);
    void HandleError(const std::string& message, const mysqlx::Error& e) const;

    mysqlx::Session* default_conn_;
    std::string default_schema_;
    std::string default_table_;

};

#endif // TALK_USER_DAO_H
