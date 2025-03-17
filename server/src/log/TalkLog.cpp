#include "TalkLog.h"
#include <iostream>
#include <iomanip>
#include <ctime>

TalkLog& TalkLog::getInstance() {
    static TalkLog instance;
    return instance;
}

TalkLog::TalkLog() : currentLevel_(LogLevel::INFO) {}

TalkLog::~TalkLog() {
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

void TalkLog::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex_);
    if (logFile_.is_open()) {
        logFile_.close();
    }
    logFile_.open(filename, std::ios::app);
}

void TalkLog::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex_);
    currentLevel_ = level;
}

void TalkLog::log(LogLevel level, const std::string& message) {
    if (level < currentLevel_) return;

    std::lock_guard<std::mutex> lock(logMutex_);

    std::string timestamp = getTimestamp();
    std::string levelStr = logLevelToString(level);

    std::string logMessage = "[" + timestamp + "] [" + levelStr + "] " + message;

    std::cout << logMessage << std::endl;

    if (logFile_.is_open()) {
        logFile_ << logMessage << std::endl;
    }
}

std::string TalkLog::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm buf;
    localtime_r(&nowTime, &buf);
    
    std::ostringstream oss;
    oss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string TalkLog::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}
