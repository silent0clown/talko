#include <gtest/gtest.h>
#include "whisp_log.h"
#include <fstream>
#include <thread>
#include <vector>

class TalkLogTest : public ::testing::Test {
protected:
    void SetUp() override {
        logFile = "test_talklog.log";
        TalkLog::getInstance().setLogFile(logFile);
    }

    void TearDown() override {
        std::remove(logFile.c_str());
    }

    std::string logFile;
};

// 测试日志级别过滤
TEST_F(TalkLogTest, LogLevelFiltering) {
    TalkLog::getInstance().setLogLevel(LogLevel::INFO);
    WHISP_LOG_DEBUG("This should not appear in log");
    WHISP_LOG_INFO("This should appear in log");
    WHISP_LOG_ERROR("This should also appear in log");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::ifstream file(logFile);
    std::string line;
    int count = 0;
    while (std::getline(file, line)) {
        count++;
    }
    file.close();
    EXPECT_EQ(count, 2);  // 只有 INFO 和 ERROR 级别的日志应被记录
}

// // 测试日志文件创建
// TEST_F(TalkLogTest, LogFileCreation) {
//     EXPECT_TRUE(std::ifstream(logFile).good());
// }

// // 测试多线程写入
// TEST_F(TalkLogTest, ConcurrentLogging) {
//     constexpr int THREAD_COUNT = 10;
//     constexpr int LOGS_PER_THREAD = 50;
//     TalkLog::getInstance().setLogLevel(LogLevel::INFO);

//     std::vector<std::thread> threads;
//     for (int i = 0; i < THREAD_COUNT; ++i) {
//         threads.emplace_back([]() {
//             for (int j = 0; j < LOGS_PER_THREAD; ++j) {
//                 WHISP_LOG_INFO("Thread log entry");
//             }
//         });
//     }
//     for (auto& t : threads) {
//         t.join();
//     }
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));

//     std::ifstream file(logFile);
//     int count = 0;
//     std::string line;
//     while (std::getline(file, line)) {
//         count++;
//     }
//     file.close();
//     EXPECT_EQ(count, THREAD_COUNT * LOGS_PER_THREAD);
// }

// // 测试日志文件写入
// TEST_F(TalkLogTest, LogFileWriting) {
//     WHISP_LOG_INFO("Test log message");
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     std::ifstream file(logFile);
//     std::string line;
//     bool found = false;
//     while (std::getline(file, line)) {
//         if (line.find("Test log message") != std::string::npos) {
//             found = true;
//             break;
//         }
//     }
//     file.close();
//     EXPECT_TRUE(found);
// }

// 测试无日志文件时的行为
TEST_F(TalkLogTest, NoLogFileErrorHandling) {
    TalkLog::getInstance().setLogFile("/forbidden_path/log.log");
    WHISP_LOG_INFO("Should not crash");
}
