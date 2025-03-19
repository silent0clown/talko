#ifndef TALK_LOG_H
#define TALK_LOG_H

#include <string>
#include <fstream>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <filesystem>

// 日志级别
enum class LogLevel {
    TRACE = 0, DEBUG = 1, INFO = 2, WARN = 3, ERROR = 4, CRITICAL = 5
};

class TalkLog {
public:
    static TalkLog& getInstance() {
        static TalkLog instance;
        return instance;
    }

    TalkLog(const TalkLog&) = delete;
    TalkLog& operator=(const TalkLog&) = delete;

    void setLogFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(logMutex_);
        if (logFile_.is_open()) logFile_.close();
        logFile_.open(filename, std::ios::out | std::ios::app);
        if (!logFile_.is_open()) std::cerr << "无法打开日志文件: " << filename << std::endl;
    }

    void setLogLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(logMutex_);
        currentLevel_ = level;
    }

    template <typename... Args>
    // void log(LogLevel level, Args&&... args) {   // ansyc
    //     if (level < currentLevel_) return;

    //     std::ostringstream oss;
    //     (oss << ... << std::forward<Args>(args));
    //     std::string logMessage = "[" + getTimestamp() + "] [" + logLevelToString(level) + "] " + oss.str();
        
    //     {
    //         std::lock_guard<std::mutex> lock(queueMutex_);
    //         logQueue_.push(logMessage);
    //     }
    //     condVar_.notify_one();
    // }
    void log(LogLevel level, const char* file, int line, Args&&... args) {
        if (level < currentLevel_) return;
    
        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        // std::string logMessage = "[" + getTimestamp() + "] [" + logLevelToString(level) + 
        // "] [" + file + ":" + std::to_string(line) + "] " + oss.str();
        // 只获取文件名，而不是完整路径
        std::string filename = std::filesystem::path(file).filename().string();

        std::string logMessage = "[" + getTimestamp() + "] [" + logLevelToString(level) + 
                                "] [" + filename + ":" + std::to_string(line) + "] " + oss.str();
                                    
        {
            std::lock_guard<std::mutex> lock(logMutex_);  // 线程安全
            std::cout << logMessage << std::endl;  // 终端输出
            if (logFile_.is_open()) {
                logFile_ << logMessage << std::endl;  
                logFile_.flush();  // 确保日志立即写入文件
            }
        }
    }

private:
    TalkLog() : currentLevel_(LogLevel::INFO), stopLogging_(false) {
        // std::filesystem::create_directories("/data/log");
        // setLogFile("/data/log/talko.log");
        logFile_.close();
        logThread_ = std::thread(&TalkLog::processLogQueue, this);
        // std::cout<< "log system init succ" << std::endl;
    }

    ~TalkLog() {
        stopLogging_ = true;
        condVar_.notify_one();
        if (logThread_.joinable()) logThread_.join();
        if (logFile_.is_open()) logFile_.close();
    }

    void processLogQueue() {
        while (!stopLogging_ || !logQueue_.empty()) {
            std::unique_lock<std::mutex> lock(queueMutex_);
            condVar_.wait(lock, [this] { return stopLogging_ || !logQueue_.empty(); });
            while (!logQueue_.empty()) {
                std::string message = logQueue_.front();
                logQueue_.pop();
                lock.unlock();
                
                std::cout << message << '\n';
                if (logFile_.is_open()) logFile_ << message << '\n';
                
                lock.lock();
            }
        }
    }

    std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);

        std::tm now_tm;
#ifdef _WIN32
        localtime_s(&now_tm, &now_time_t);
#else
        localtime_r(&now_time_t, &now_tm);
#endif

        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &now_tm);
        return std::string(buffer);
    }

    std::string logLevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE:    return "TRACE";
            case LogLevel::DEBUG:    return "DEBUG";
            case LogLevel::INFO:     return "INFO";
            case LogLevel::WARN:     return "WARN";
            case LogLevel::ERROR:    return "ERROR";
            case LogLevel::CRITICAL: return "CRITICAL";
            default:                 return "UNKNOWN";
        }
    }

    std::ofstream logFile_;
    LogLevel currentLevel_;
    std::mutex logMutex_;
    std::mutex queueMutex_;
    std::condition_variable condVar_;
    std::queue<std::string> logQueue_;
    std::thread logThread_;
    bool stopLogging_;
};

#define TALKO_LOG_TRACE(...)    TalkLog::getInstance().log(LogLevel::TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define TALKO_LOG_DEBUG(...)    TalkLog::getInstance().log(LogLevel::DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define TALKO_LOG_INFO(...)     TalkLog::getInstance().log(LogLevel::INFO, __FILE__, __LINE__, __VA_ARGS__)
#define TALKO_LOG_WARN(...)     TalkLog::getInstance().log(LogLevel::WARN, __FILE__, __LINE__, __VA_ARGS__)
#define TALKO_LOG_ERROR(...)    TalkLog::getInstance().log(LogLevel::ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define TALKO_LOG_CRITICAL(...) TalkLog::getInstance().log(LogLevel::CRITICAL, __FILE__, __LINE__, __VA_ARGS__)

#endif // TALK_LOG_H
