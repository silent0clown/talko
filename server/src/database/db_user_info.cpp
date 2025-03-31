#include "db_user_info.h"
#include <chrono>
#include <iomanip>
#include <sstream>

// 获取当前时间（ISO 8601格式）
static std::string GetCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}


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

bool WhispUserDAO::InsertUser(const UserInfo& user,
                            mysqlx::Session* conn_override,
                            const std::string& schema_override,
                            const std::string& table_override) 
{
    mysqlx::Session* conn;
    std::string schema, table;
    ResolveParams(&conn, &schema, &table, conn_override, schema_override, table_override);

    if (!conn || schema.empty() || table.empty()) {
        TALKO_LOG_WARN("Invalid database parameters");
        return false;
    }

    try {
        mysqlx::Table userTable = conn->getSchema(schema).getTable(table);
        userTable.insert(
            "username", "password_hash", "nickname", "avatar_url", "gender",
            "birthday", "phone_number", "email", "region", "status", "last_login_time"
        ).values(
            user.username, user.password_hash, user.nickname, user.avatar_url,
            user.gender, user.birthday, user.phone_number, user.email,
            user.region, user.status, GetCurrentDateTime()
        ).execute();

        TALKO_LOG_DEBUG("Inserted user: " + user.username);
        return true;
    } catch (const mysqlx::Error& e) {
        HandleError("Insert failed: ", e);
        return false;
    }
}

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
std::unique_ptr<UserInfo> WhispUserDAO::GetUserById(
    int user_id,
    mysqlx::Session* conn_override,
    const std::string& schema_override,
    const std::string& table_override) 
{
    mysqlx::Session* conn;
    std::string schema, table;
    ResolveParams(&conn, &schema, &table, conn_override, schema_override, table_override);

    if (!conn || schema.empty() || table.empty()) {
        TALKO_LOG_WARN("Invalid database parameters");
        return nullptr;
    }

    try {
        auto row = conn->getSchema(schema)
            .getTable(table)
            .select("*")
            .where("id = :id")
            .bind("id", user_id)
            .execute()
            .fetchOne();

        if (!row.isNull()) {
            return std::make_unique<UserInfo>(ParseUser(row));
        }
    } catch (const mysqlx::Error& e) {
        HandleError("Query failed: ", e);
    }
    return nullptr;
}

// 更新用户信息
bool WhispUserDAO::UpdateUser(const UserInfo& user, mysqlx::Session* session, const std::string& DBschema, const std::string& DBtable) {
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
bool WhispUserDAO::DeleteUser(int user_id, mysqlx::Session* session, const std::string& DBschema, const std::string& DBtable) {
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

// 解析数据库行到UserInfo结构体
UserInfo WhispUserDAO::ParseUser(const mysqlx::Row& row) {
    UserInfo user;
    try {
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
    } catch (const mysqlx::Error& e) {
        HandleError("Parse user failed: ", e);
        throw;
    }
    return user;
}

// 统一错误处理
void WhispUserDAO::HandleError(const std::string& message, const mysqlx::Error& e) const {
    TALKO_LOG_ERROR(message + e.what());
}