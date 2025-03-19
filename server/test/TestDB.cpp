#include <gtest/gtest.h>
#include "MySQLConnectionPool.h"
#include <thread>
#include <vector>

// 测试连接池初始化
// 在单例模式下，GetInstance() 返回的实例通常在程序退出时才会销毁
TEST(MySQLConnectionPoolTest, Initialization) {
    try {
        MySQLConnectionPool::GetInstance()->Init("127.0.0.1", "root", "talko_root", "talko_server", 6603, 5);
        // 这里假设连接池的队列大小能反映连接数量
        EXPECT_EQ(MySQLConnectionPool::GetInstance()->GetConnectionPoolSize(), 5);
    } catch (const std::exception& e) {
        GTEST_FAIL() << "Connection pool initialization failed: " << e.what();
    }
}

// 测试获取连接
TEST(MySQLConnectionPoolTest, GetConnection) {
    MySQLConnectionPool::GetInstance()->Init("127.0.0.1", "root", "talko_root", "talko_server", 6603, 5);
    auto conn = MySQLConnectionPool::GetInstance()->GetConnection();
    EXPECT_NE(conn, nullptr);
    // 归还连接
    MySQLConnectionPool::GetInstance()->ReleaseConnection(std::move(conn));
}

// 测试归还连接
TEST(MySQLConnectionPoolTest, ReleaseConnection) {
    MySQLConnectionPool::GetInstance()->Init("127.0.0.1", "root", "talko_root", "talko_server", 6603, 5);
    auto conn = MySQLConnectionPool::GetInstance()->GetConnection();
    size_t poolSizeBeforeRelease = MySQLConnectionPool::GetInstance()->GetConnectionPoolSize();
    MySQLConnectionPool::GetInstance()->ReleaseConnection(std::move(conn));
    size_t poolSizeAfterRelease = MySQLConnectionPool::GetInstance()->GetConnectionPoolSize();
    EXPECT_EQ(poolSizeAfterRelease, poolSizeBeforeRelease + 1);
}

// 多线程并发测试
void threadFunction() {
    try {
        auto conn = MySQLConnectionPool::GetInstance()->GetConnection();
        // 模拟使用连接
        if (conn) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            MySQLConnectionPool::GetInstance()->ReleaseConnection(std::move(conn)); // std::move(conn) 将连接的所有权转移，确保 ReleaseConnection() 里不会拷贝，而是直接操作原对象，提高效率
        }
    } catch (const std::exception& e) {
        std::cerr << "获取连接失败: " << e.what() << std::endl;
    }
}

TEST(MySQLConnectionPoolTest, ConcurrentAccess) {
    MySQLConnectionPool::GetInstance()->Init("127.0.0.1", "root", "talko_root", "talko_server", 6603, 5);
    std::cout << "cur size : " << MySQLConnectionPool::GetInstance << std::endl;
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(threadFunction);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 检查连接池最终状态
    EXPECT_EQ(MySQLConnectionPool::GetInstance()->GetConnectionPoolSize(), 5);
}
