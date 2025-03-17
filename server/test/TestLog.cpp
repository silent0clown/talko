#include "gtest/gtest.h"
#include "TalkLog.h"
#include <fstream>
#include <sstream>

// Test case for the TalkLog singleton
TEST(TalkLogTest, SingletonInstance) {
    TalkLog& log1 = TalkLog::getInstance();
    TalkLog& log2 = TalkLog::getInstance();
    
    // Ensure both instances are the same
    EXPECT_EQ(&log1, &log2);
}

// Test case for setting log file
TEST(TalkLogTest, SetLogFile) {
    TalkLog& log = TalkLog::getInstance();
    log.setLogFile("test.log");

    std::ifstream logFile("test.log");
    ASSERT_TRUE(logFile.is_open());

    logFile.close();
    remove("test.log");  // Clean up
}

// Test case for logging to console and file
TEST(TalkLogTest, LogToConsoleAndFile) {
    TalkLog& log = TalkLog::getInstance();
    log.setLogFile("test.log");
    log.setLogLevel(LogLevel::DEBUG);

    // Redirecting the std::cout to capture the console output
    std::ostringstream consoleOutput;
    std::streambuf* originalCout = std::cout.rdbuf(consoleOutput.rdbuf());

    // Log messages
    LOG_INFO("Test log message");
    LOG_WARN("Test warning message");

    // Check if the console output contains the log message
    EXPECT_NE(consoleOutput.str().find("Test log message"), std::string::npos);
    EXPECT_NE(consoleOutput.str().find("Test warning message"), std::string::npos);

    // Check if the file contains the log message
    std::ifstream logFile("test.log");
    std::string fileContent((std::istreambuf_iterator<char>(logFile)),
                            std::istreambuf_iterator<char>());
    
    EXPECT_NE(fileContent.find("Test log message"), std::string::npos);
    EXPECT_NE(fileContent.find("Test warning message"), std::string::npos);

    // Clean up
    logFile.close();
    std::cout.rdbuf(originalCout);  // Restore std::cout
    remove("test.log");
}

// Test case for logging level filtering
TEST(TalkLogTest, LogLevelFiltering) {
    TalkLog& log = TalkLog::getInstance();
    log.setLogLevel(LogLevel::ERROR);

    std::ostringstream consoleOutput;
    std::streambuf* originalCout = std::cout.rdbuf(consoleOutput.rdbuf());

    // Should log only ERROR and CRITICAL messages
    LOG_INFO("Info message");
    LOG_WARN("Warning message");
    LOG_ERROR("Error message");

    // Check if the console output contains only ERROR message
    EXPECT_EQ(consoleOutput.str().find("Error message"), std::string::npos);

    std::cout.rdbuf(originalCout);  // Restore std::cout
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
