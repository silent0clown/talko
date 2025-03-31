#ifndef WHISP_USER_DAO_H
#define WHISP_USER_DAO_H

#include "db_mysqlconn_pool.h"

class WhispUserDAO {
public:
    explicit WhispUserDAO(WhispMysqlConnPool& conn_pool) : conn_pool_(conn_pool) {}

    // 示例：查询用户信息
    std::string get_user_info(int user_id) {
        auto conn = conn_pool_.get_connection();
        if (!conn) {
            return "DB Connection Failed";
        }

        std::string query = "SELECT username FROM users WHERE id = " + std::to_string(user_id);
        auto result = conn->execute_query(query);
        if (result.next()) {
            return result.get_string("username");
        }
        return "User Not Found";
    }

private:
    WhispMysqlConnPool& conn_pool_;
};

#endif // WHISP_USER_DAO_H
