#ifndef TALK_USER_DAO_H
#define TALK_USER_DAO_H

#include <mysqlx/xdevapi.h>
#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <ctime>
#include "MySQLConnectionPool.h"
#include "TalkLog.h"

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

class MySQLConnectionGuard {
public:
    MySQLConnectionGuard() {
        session_ = MySQLConnectionPool::GetInstance()->GetConnection();
    }

    ~MySQLConnectionGuard() {
        if (session_) {
            MySQLConnectionPool::GetInstance()->ReleaseConnection(std::move(session_));
        }
    }

    mysqlx::Session* GetConnection() {
        return session_.get();
    }

private:
    std::unique_ptr<mysqlx::Session> session_;
};


// 获取当前时间并转换为字符串
std::string getCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

class TalkUserDAO {
public:
    // 插入新用户
    /*
    为什么要用引用 &？
        如果不加 &，比如 string DBtable，每次调用函数时都会拷贝一份 DBtable，开销较大。
        加 & 后，参数传递的是原对象的引用，避免拷贝，提高性能。
        const 关键字保证参数不会在函数内部被修改，防止意外修改。
    
    在 C++ 里：
        **左值（L-value）：**可以取地址的变量，例如 string s = "hello";
        **右值（R-value）：**不能取地址的临时值，例如 "hello"

    如果没有 const，比如：
        bool InsertUser(UserInfo& user, string& DBtable);
    那么 InsertUser(user, "user_info") 就会报错，因为右值不能绑定到非常量引用 string&。

    当 const string& DBtable 作为参数时，既可以接受左值，也可以接受右值:
        string table1 = "user_info";
        InsertUser(user, table1);  // 传左值，可以绑定到 const string& DBtable
        InsertUser(user, "user_info");  // 传右值也可以，因为是 const 引用
    */
    bool InsertUser(mysqlx::Session* DBconn, const std::string& DBschema, const std::string& DBtable, const UserInfo& user) {
        if (!DBconn || DBschema.empty() || DBtable.empty()) {
            // std::cerr << "Invalid database connection or schema/table name" << std::endl;
            TALKO_LOG_WARN("Invalid database connection or schema/table name");
            return false;
        }

        try {
            // 获取目标数据库和表
            mysqlx::Schema db = DBconn->getSchema(DBschema);
            mysqlx::Table userTable = db.getTable(DBtable);

            // // 检查索引信息
            // mysqlx::RowResult indexResult = DBconn->sql("SHOW INDEX FROM " + DBschema + "." + DBtable).execute();
            // for (mysqlx::Row row : indexResult.fetchAll()) {
            //     std::string indexName = row[2]; // 索引名称
            //     std::string columnName = row[4]; // 列名称

            //     if (indexName == "username_index" && columnName != "username") {
            //         std::cerr << "Index mismatch with UserInfo" << std::endl;
            //         return false;
            //     }
            //     // 可以添加对其他索引的校验
            // }

            // 准备插入的数据
            std::string current_datetime = getCurrentDateTime();

            // 执行插入操作
            userTable.insert("username", "password_hash", "nickname", "avatar_url", "gender", "birthday", 
                             "phone_number", "email", "region", "status", "last_login_time")
                     .values(user.username, user.password_hash, user.nickname, user.avatar_url, user.gender, 
                             user.birthday, user.phone_number, user.email, user.region, user.status, current_datetime)
                     .execute();

            TALKO_LOG_DEBUG("Insert user: " + user.username + " success");
            return true;
        } catch (const mysqlx::Error& e) {
            HandleError("Error inserting user: ", e);
            return false;
        }
    }
    

    // 查询用户
    /* 返回指针还是结构体的思考：
        如果返回结构体，如果 UserInfo 结构体很大，返回时会涉及 值拷贝，在高性能场景下可能有一定的性能损耗

        如果返回指针，不会产生结构体的拷贝开销，适合返回值很大、且需要高性能的场景
        适合多态，如果UserInfo可能有多个子类，指针更适合支持多态

        C++代码推荐使用指针指针，避免手动delete的问题
        | 特性                | `std::unique_ptr`                          | `std::shared_ptr`                          |
        |---------------------|--------------------------------------------|--------------------------------------------|
        | **所有权**          | 独占                                       | 共享                                       |
        | **引用计数**        | 无                                         | 有                                         |
        | **拷贝**            | ❌ 禁止拷贝                                 | ✅ 允许拷贝                                 |
        | **移动**            | ✅ 可用 `std::move`                         | ✅ 可用 `std::move`                         |
        | **性能**            | ✅ 轻量，无额外开销                         | ❌ 额外的引用计数管理                       |
        | **适用场景**        | 独占资源                                   | 需要多个地方共享资源                       |
        | **常见陷阱**        | 不能拷贝                                   | 循环引用导致内存泄漏                       |
    */
    std::unique_ptr<UserInfo> GetUserById(mysqlx::Session* session, const std::string& DBschema, const std::string& DBtable, int user_id) {
        if (!session || DBschema.empty() || DBtable.empty()) {
            TALKO_LOG_WARN("Invalid database session");
            return nullptr;
        }
    
        try {
            mysqlx::Schema db = session->getSchema(DBschema);
            mysqlx::Table userTable = db.getTable(DBtable);
    
            mysqlx::RowResult result = userTable.select("*")
                                                .where("id = :id")
                                                .bind("id", user_id)
                                                .execute();
    
            mysqlx::Row row = result.fetchOne();
            if (!row.isNull()) {
                auto user = std::make_unique<UserInfo>(ParseUser(row));
                TALKO_LOG_DEBUG("Fetched user: " + user->username);
                return user;
            } else {
                TALKO_LOG_INFO("User ID " + std::to_string(user_id) + " not found.");
            }
        } catch (const mysqlx::Error& e) {
            HandleError("Error fetching user: ", e);
        }
    
        return nullptr;
    }
    

    // 更新用户信息
    bool UpdateUser(mysqlx::Session* session, const std::string& DBschema, const std::string& DBtable, const UserInfo& user) {
        if (!session || DBschema.empty() || DBtable.empty()) {
            TALKO_LOG_WARN("Invalid database session");
            return false;
        }

        try {
            // 获取数据库和表
            mysqlx::Schema db = session->getSchema(DBschema);  // 数据库名
            mysqlx::Table userTable = db.getTable(DBtable);    // 表名

            // 更新用户信息，时间字段作为字符串处理
            userTable.update()
                    .set("nickname", user.nickname)
                    .set("avatar_url", user.avatar_url)
                    .set("phone_number", user.phone_number)
                    .set("email", user.email)
                    .set("region", user.region)
                    .set("status", user.status)
                    .set("last_login_time", "") // 如果需要其他默认值，可以在这里调整
                    .where("id = :id")
                    .bind("id", user.id)
                    .execute();

            TALKO_LOG_INFO("User with ID " + std::to_string(user.id) + " updated successfully.");
            return true;
        } catch (const mysqlx::Error& e) {
            HandleError("Error updating user: ", e);
            return false;
        }
    }

    // 删除用户
    bool DeleteUser(mysqlx::Session* session, const std::string& DBschema, const std::string& DBtable, int user_id) {
        if (!session || DBschema.empty() || DBtable.empty()) {
            TALKO_LOG_WARN("Invalid database session");
            return false;
        }

        try {
            // 获取数据库和表
            mysqlx::Schema db = session->getSchema(DBschema);  // 数据库名
            mysqlx::Table userTable = db.getTable(DBtable);    // 表名

            // 删除用户
            userTable.remove().where("id = :id").bind("id", user_id).execute();

            TALKO_LOG_INFO("User with ID " + std::to_string(user_id) + " deleted successfully.");
            return true;
        } catch (const mysqlx::Error& e) {
            HandleError("Error deleting user: ", e);
            return false;
        }
    }

private:
    // 解析 MySQL 结果集到 UserInfo 结构体
    UserInfo ParseUser(const mysqlx::Row& row) {
        UserInfo user;
        user.id = row[0];
        user.username = row[1].get<std::string>();
        user.password_hash = row[2].get<std::string>();
        user.nickname = row[3].get<std::string>();
        user.avatar_url = row[4].get<std::string>();
        user.gender = row[5].get<std::string>();
        user.birthday = row[6].get<std::string>();
        user.phone_number = row[7].get<std::string>();
        user.email = row[8].get<std::string>();
        user.region = row[9].get<std::string>();
        user.status = row[10].get<std::string>();
        user.last_login_time = row[11].get<std::string>();
        user.create_time = row[12].get<std::string>();
        user.update_time = row[13].get<std::string>();
        return user;
    }

    // 处理错误
    void HandleError(const std::string& message, const mysqlx::Error& e) {
        TALKO_LOG_ERROR(message + e.what());
        // std::cerr << message << e.what() << std::endl;
    }
};

#endif // TALK_USER_DAO_H
