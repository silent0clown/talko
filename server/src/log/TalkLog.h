#ifndef TALK_LOG_H
#define TALK_LOG_H

#include <string>
#include <memory>
#include <fstream>
#include <mutex>

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL
};

class TalkLog {
public:
    static TalkLog& getInstance();

    void setLogFile(const std::string& filename);
    void setLogLevel(LogLevel level);
    
    void log(LogLevel level, const std::string& message);

private:
    TalkLog();
    ~TalkLog();
    
    std::ofstream logFile_;
    LogLevel currentLevel_;
    std::mutex logMutex_;

    std::string getTimestamp();
    std::string logLevelToString(LogLevel level);
};

#define LOG_TRACE(msg) TalkLog::getInstance().log(LogLevel::TRACE, msg)
#define LOG_DEBUG(msg) TalkLog::getInstance().log(LogLevel::DEBUG, msg)
#define LOG_INFO(msg)  TalkLog::getInstance().log(LogLevel::INFO, msg)
#define LOG_WARN(msg)  TalkLog::getInstance().log(LogLevel::WARN, msg)
#define LOG_ERROR(msg) TalkLog::getInstance().log(LogLevel::ERROR, msg)
#define LOG_CRITICAL(msg) TalkLog::getInstance().log(LogLevel::CRITICAL, msg)

#endif // TALK_LOG_H
