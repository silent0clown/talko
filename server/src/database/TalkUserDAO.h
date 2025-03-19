#ifndef TALK_USER_DAO_H
#define TALK_USER_DAO_H

#include <mysqlx/xdevapi.h>
#include <string>
#include <vector>
#include <iostream>
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

class TalkUserDAO {
public:
    // 插入新用户
    bool InsertUser(const UserInfo& user) {
        MySQLConnectionGuard guard;
        mysqlx::Session* session = guard.GetConnection();
        if (!session) return false;

        try {
            mysqlx::Schema db = session->getSchema("talko_server");  // 数据库名
            mysqlx::Table userTable = db.getTable("user_info");

            // 插入数据，日期和时间字段作为字符串处理
            userTable.insert("username", "password_hash", "nickname", "avatar_url", "gender", "birthday", 
                             "phone_number", "email", "region", "status", "last_login_time")
                     .values(user.username, user.password_hash, user.nickname, user.avatar_url, user.gender, 
                             user.birthday, user.phone_number, user.email, user.region, user.status, "")
                     .execute();
            TALKO_LOG_DEBUG("Insert user: " + user.username + " success");
            return true;
        } catch (const mysqlx::Error& e) {
            HandleError("Error inserting user: ", e);
            return false;
        }
    }

    // 查询用户
    UserInfo GetUserById(int user_id) {
        UserInfo user;
        MySQLConnectionGuard guard;
        mysqlx::Session* session = guard.GetConnection();
        if (!session) return user;

        try {
            mysqlx::Schema db = session->getSchema("talko_server");  // 数据库名
            mysqlx::Table userTable = db.getTable("user_info");

            // 查询用户
            mysqlx::RowResult result = userTable.select("*")
                                                .where("id = :id")
                                                .bind("id", user_id)
                                                .execute();

            if (result.count() > 0) {
                mysqlx::Row row = result.fetchOne();
                user = ParseUser(row);
            }
        } catch (const mysqlx::Error& e) {
            HandleError("Error fetching user: ", e);
        }

        return user;
    }

    // 更新用户信息
    bool UpdateUser(const UserInfo& user) {
        MySQLConnectionGuard guard;
        mysqlx::Session* session = guard.GetConnection();
        if (!session) return false;

        try {
            mysqlx::Schema db = session->getSchema("talko_server");  // 数据库名
            mysqlx::Table userTable = db.getTable("user_info");

            // 更新用户信息，时间字段作为字符串处理
            userTable.update()
                    .set("nickname", user.nickname)
                    .set("avatar_url", user.avatar_url)
                    .set("phone_number", user.phone_number)
                    .set("email", user.email)
                    .set("region", user.region)
                    .set("status", user.status)
                    .set("last_login_time", "")
                    .where("id = :id")
                    .bind("id", user.id)
                    .execute();

            return true;
        } catch (const mysqlx::Error& e) {
            HandleError("Error updating user: ", e);
            return false;
        }
    }

    // 删除用户
    bool DeleteUser(int user_id) {
        MySQLConnectionGuard guard;
        mysqlx::Session* session = guard.GetConnection();
        if (!session) return false;

        try {
            mysqlx::Schema db = session->getSchema("talko_server");  // 数据库名
            mysqlx::Table userTable = db.getTable("user_info");

            // 删除用户
            userTable.remove().where("id = :id").bind("id", user_id).execute();

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
        std::cerr << message << e.what() << std::endl;
    }
};

#endif // TALK_USER_DAO_H
