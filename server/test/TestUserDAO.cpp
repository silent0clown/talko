#include <gtest/gtest.h>
#include "TalkUserDAO.h"  // 假设你的 TalkUserDAO 和相关代码文件在此
#include "MySQLConnectionPool.h"  // 需要包含 MySQL 连接池头文件

// 使用 GoogleTest 测试用例
class TalkUserDAOTest : public ::testing::Test {
protected:
    TalkUserDAO dao;  // 我们将测试的对象

    // 设置工作（如果有必要）
    void SetUp() override {
        // 可选：初始化数据库连接池等
    }

    // 清理工作（如果有必要）
    void TearDown() override {
        // 可选：清理工作，例如删除插入的测试数据
    }
};

TEST_F(TalkUserDAOTest, TestInsertUser_ValidData) {
    UserInfo user = {
        0, "validuser", "hashedpassword", "nickname", "avatar_url", "male", 
        "1990-01-01", "1234567890", "user@example.com", "Beijing", "active", 
        "2025-03-19 10:00:00", "2025-03-19 10:00:00", "2025-03-19 10:00:00"
    };

    bool result = dao.InsertUser(user);
    
    // 确保插入成功
    ASSERT_TRUE(result);

    // 查询插入的用户并验证数据
    UserInfo insertedUser = dao.GetUserById(1);  // 假设 ID 为 1
    ASSERT_EQ(insertedUser.username, "validuser");
    ASSERT_EQ(insertedUser.email, "user@example.com");
}

TEST_F(TalkUserDAOTest, TestInsertUser_DuplicateUsername) {
    UserInfo user = {
        0, "duplicateuser", "hashedpassword", "nickname", "avatar_url", "female", 
        "1990-01-01", "1234567890", "duplicate@example.com", "Shanghai", "active", 
        "2025-03-19 10:00:00", "2025-03-19 10:00:00", "2025-03-19 10:00:00"
    };

    dao.InsertUser(user);  // 插入第一个用户

    // 尝试插入一个重复的用户名
    UserInfo duplicateUser = {
        0, "duplicateuser", "newhashedpassword", "newnickname", "newavatar", "male", 
        "1995-02-02", "9876543210", "duplicate@example.com", "Beijing", "inactive", 
        "2025-03-19 10:00:00", "2025-03-19 10:00:00", "2025-03-19 10:00:00"
    };

    bool result = dao.InsertUser(duplicateUser);

    // 插入失败，返回值应该是 false
    ASSERT_FALSE(result);
}

TEST_F(TalkUserDAOTest, TestInsertUser_MissingRequiredField) {
    UserInfo user = {
        0, "", "hashedpassword", "nickname", "avatar_url", "male", 
        "1990-01-01", "1234567890", "user@example.com", "Beijing", "active", 
        "2025-03-19 10:00:00", "2025-03-19 10:00:00", "2025-03-19 10:00:00"
    };

    bool result = dao.InsertUser(user);

    // 用户名为空，插入应该失败
    ASSERT_FALSE(result);
}

TEST_F(TalkUserDAOTest, TestInsertUser_DatabaseConnectionFailure) {
    // 模拟数据库连接失败
    // dao.SetDatabaseConnectionFailure(true);  // 假设你有这种机制来模拟连接失败

    UserInfo user = {
        0, "validuser", "hashedpassword", "nickname", "avatar_url", "male", 
        "1990-01-01", "1234567890", "user@example.com", "Beijing", "active", 
        "2025-03-19 10:00:00", "2025-03-19 10:00:00", "2025-03-19 10:00:00"
    };

    bool result = dao.InsertUser(user);

    // 由于数据库连接失败，插入操作应当失败
    ASSERT_FALSE(result);
}

TEST_F(TalkUserDAOTest, TestInsertUser_EmptyDateField) {
    UserInfo user = {
        0, "validuser", "hashedpassword", "nickname", "avatar_url", "male", 
        "1990-01-01", "1234567890", "user@example.com", "Beijing", "active", 
        "", "", ""  // 空的日期字段
    };

    bool result = dao.InsertUser(user);

    // 插入成功，日期字段应该被正确填充
    ASSERT_TRUE(result);

    // 获取插入的用户，验证日期字段是否被填充（如当前时间）
    UserInfo insertedUser = dao.GetUserById(1);
    ASSERT_FALSE(insertedUser.last_login_time.empty());
}
