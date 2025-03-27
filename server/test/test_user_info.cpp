#include <gtest/gtest.h>
#include "db_user_info.h"

class TalkUserDAOTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化数据库连接
        session_guard = std::make_unique<MySQLConnectionGuard>();
        session = session_guard->GetConnection();
        DBschema = "talko_server";
        DBtable = "test_user";

        if (!session) {
            FAIL() << "Database connection failed!";
        }

        // 创建测试表
        try {
            session->sql("DROP TABLE IF EXISTS " + DBschema + "." + DBtable).execute();
            session->sql(
                "CREATE TABLE " + DBschema + "." + DBtable + " ("
                "id INT AUTO_INCREMENT PRIMARY KEY, "
                "username VARCHAR(255) NOT NULL UNIQUE, "
                "password_hash VARCHAR(255) NOT NULL, "
                "nickname VARCHAR(255), "
                "avatar_url TEXT, "
                "gender ENUM('male', 'female', 'other'), "
                "birthday DATE, "
                "phone_number VARCHAR(20), "
                "email VARCHAR(255), "
                "region VARCHAR(255), "
                "status ENUM('active', 'inactive', 'banned') NOT NULL DEFAULT 'active', "
                "last_login_time DATETIME, "
                "create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                "update_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP"
                ")").execute();

            // 确保表创建成功
            session->sql("SELECT 1 FROM " + DBschema + "." + DBtable + " LIMIT 1").execute();
        } catch (const mysqlx::Error& e) {
            FAIL() << "Failed to create test table: " << e.what();
        }
    }

    void TearDown() override {
        // 清理测试数据
        if (session) {
            try {
                session->sql("DROP TABLE IF EXISTS " + DBschema + "." + DBtable).execute();
            } catch (const mysqlx::Error& e) {
                std::cerr << "Failed to drop test table: " << e.what() << std::endl;
            }
        }
    }

    std::unique_ptr<MySQLConnectionGuard> session_guard;
    mysqlx::Session* session;
    std::string DBschema;
    std::string DBtable;
    TalkUserDAO dao;
};

// 测试正常插入用户
TEST_F(TalkUserDAOTest, InsertUser_Success) {
    UserInfo user = {0, "test_user", "hashed_password", "Test Nickname", "avatar_url",
                     "male", "1990-01-01", "1234567890", "test@example.com",
                     "region", "active", "", "", ""};
    
    bool result = dao.InsertUser(session, DBschema, DBtable, user);
    EXPECT_TRUE(result);
    // 查询数据库验证数据
    try {
        mysqlx::RowResult result = session->sql("SELECT gender, phone_number FROM " + DBschema + "." + DBtable +
                                                " WHERE username = 'test_user'").execute();

        // 确保查到了数据
        mysqlx::Row row = result.fetchOne();
        ASSERT_FALSE(row.isNull());  // 断言查询结果不为空

        // 验证 gender
        std::string db_gender = row[0].get<std::string>();
        EXPECT_EQ(db_gender, "male") << "Gender does not match! Expected: male, Got: " << db_gender;

        // 验证 phone_number
        std::string db_phone = row[1].get<std::string>();
        EXPECT_EQ(db_phone, "1234567890") << "Phone number does not match! Expected: 1234567890, Got: " << db_phone;
    
    } catch (const mysqlx::Error& e) {
        FAIL() << "Database query failed: " << e.what();
    }
}

// 测试数据库连接为空
TEST_F(TalkUserDAOTest, InsertUser_NullDBConnection) {
    UserInfo user = {0, "test_user", "hashed_password", "Test Nickname", "avatar_url",
                     "male", "1990-01-01", "1234567890", "test@example.com",
                     "region", "active", "", "", ""};
    
    bool result = dao.InsertUser(nullptr, DBschema, DBtable, user);
    EXPECT_FALSE(result);
}

// 测试 Schema 为空
TEST_F(TalkUserDAOTest, InsertUser_EmptySchema) {
    UserInfo user = {0, "test_user", "hashed_password", "Test Nickname", "avatar_url",
                     "male", "1990-01-01", "1234567890", "test@example.com",
                     "region", "active", "", "", ""};
    
    bool result = dao.InsertUser(session, "", DBtable, user);
    EXPECT_FALSE(result);
}

// 测试 Table 为空
TEST_F(TalkUserDAOTest, InsertUser_EmptyTable) {
    UserInfo user = {0, "test_user", "hashed_password", "Test Nickname", "avatar_url",
                     "male", "1990-01-01", "1234567890", "test@example.com",
                     "region", "active", "", "", ""};
    
    bool result = dao.InsertUser(session, DBschema, "", user);
    EXPECT_FALSE(result);
}



// 测试根据 ID 查询用户
TEST_F(TalkUserDAOTest, GetUserById_Success) {
    // 先插入一个用户
    UserInfo user = {0, "test_user", "hashed_password", "Test Nickname", "avatar_url",
                     "male", "1990-01-01", "1234567890", "test@example.com",
                     "region", "active", "", "", ""};

    bool insert_result = dao.InsertUser(session, DBschema, DBtable, user);
    ASSERT_TRUE(insert_result) << "Failed to insert user before testing GetUserById";

    // 获取插入用户的 ID
    int user_id = -1;
    try {
        mysqlx::RowResult result = session->sql("SELECT id FROM " + DBschema + "." + DBtable +
                                                " WHERE username = 'test_user'").execute();
        mysqlx::Row row = result.fetchOne();
        ASSERT_FALSE(row.isNull()) << "Failed to fetch inserted user ID";
        user_id = row[0].get<int>();
    } catch (const mysqlx::Error& e) {
        FAIL() << "Database query failed: " << e.what();
    }

    // 调用 GetUserById 查询用户信息
    std::unique_ptr<UserInfo> fetched_user = dao.GetUserById(session, DBschema, DBtable, user_id);

    // 验证查询结果
    EXPECT_EQ(fetched_user->username, "test_user") << "Username does not match!";
    EXPECT_EQ(fetched_user->password_hash, "hashed_password") << "Password hash does not match!";
    EXPECT_EQ(fetched_user->nickname, "Test Nickname") << "Nickname does not match!";
    EXPECT_EQ(fetched_user->gender, "male") << "Gender does not match!";
    EXPECT_EQ(fetched_user->phone_number, "1234567890") << "Phone number does not match!";
    EXPECT_EQ(fetched_user->email, "test@example.com") << "Email does not match!";
    EXPECT_EQ(fetched_user->region, "region") << "Region does not match!";
    EXPECT_EQ(fetched_user->status, "active") << "Status does not match!";
}

// 测试查询不存在的用户
TEST_F(TalkUserDAOTest, GetUserById_NotFound) {
    int non_existent_id = 99999;  // 一个极大可能不存在的 ID

    std::unique_ptr<UserInfo> fetched_user = dao.GetUserById(session, DBschema, DBtable, non_existent_id);
    
    // 验证返回值，假设 UserInfo 默认构造时 username 为空
    ASSERT_EQ(fetched_user, nullptr) << "Expected nullptr for non-existent user";
}

