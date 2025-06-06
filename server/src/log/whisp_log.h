#ifndef WHISP_LOG_H
#define WHISP_LOG_H
/*
* 异步日志类
*/

#include <stdio.h>
#include <string>
#include <list>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>


#define WHISP_LOG_API

// 日志级别
enum LOG_LEVEL {
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,    //用于业务错误
    LOG_LEVEL_SYSERROR, //用于技术框架本身的错误
    LOG_LEVEL_FATAL,    //FATAL 级别的日志会让在程序输出日志后退出
    LOG_LEVEL_CRITICAL  //CRITICAL 日志不受日志级别控制，总是输出
};

//TODO: 多增加几个策略
//注意：如果打印的日志信息中有中文，则格式化字符串要用_T()宏包裹起来，
//e.g. LOGI(_T("GroupID=%u, GroupName=%s, GroupName=%s."), lpGroupInfo->m_nGroupCode, lpGroupInfo->m_strAccount.c_str(), lpGroupInfo->m_strName.c_str());
#define WHISP_LOG_TRACE(...)      WhispLog::get_instance().log_output(LOG_LEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define WHISP_LOG_DEBUG(...)      WhispLog::get_instance().log_output(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define WHISP_LOG_INFO(...)       WhispLog::get_instance().log_output(LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define WHISP_LOG_WARN(...)       WhispLog::get_instance().log_output(LOG_LEVEL_WARNING, __FILE__, __LINE__,__VA_ARGS__)
#define WHISP_LOG_ERROR(...)      WhispLog::get_instance().log_output(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define WHISP_LOG_SYSERROR(...)   WhispLog::get_instance().log_output(LOG_LEVEL_SYSERROR, __FILE__, __LINE__, __VA_ARGS__)
#define WHISP_LOG_FALTAL(...)     WhispLog::get_instance().log_output(LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)        //为了让FATAL级别的日志能立即crash程序，采取同步写日志的方法
#define WHISP_LOG_CRITICAL(...)   WhispLog::get_instance().log_output(LOG_LEVEL_CRITICAL, __FILE__, __LINE__, __VA_ARGS__)     //关键信息，无视日志级别，总是输出

//用于输出数据包的二进制格式
#define WHISP_LOG_DEBUG_BIN(buf, buflength) WhispLog::outputBinary(buf, buflength)

/**
 * @class WhispLog
 * @brief 日志工具类，用于管理日志输出、日志级别和日志文件操作。
 * 
 * 该类提供静态方法用于初始化日志系统、设置日志级别、写入日志消息以及管理日志文件。
 * 支持日志文件滚动、长日志行截断以及二进制数据日志等功能。
 * 该类设计为不可实例化和不可拷贝。
 */
class WhispLog {
public:
    static WhispLog& get_instance() {
        static WhispLog instance;
        return instance;
    }

    bool log_init(const char* log_file_name = nullptr, bool truncate_flag = false, int64_t roll_size = 10 * 1024 * 1024);

    void log_uninit();

    void log_set_level(LOG_LEVEL levelv);

    bool log_isrunning();

    bool log_output(LOG_LEVEL levelv, const char* fmt, ...);

    bool log_output(LOG_LEVEL levelv, const char* file_name, int line_num, const char* fmt, ...);

    bool log_output_binary(unsigned char* buffer, size_t size);

private:

    void set_line_perfix(LOG_LEVEL levelv, std::string& prefix_str);


    void get_time(char* ts, int ts_len);

    bool create_file(const char* log_file_name);

    bool write2file(const std::string& data);

    void crash();
    const char* ull2str(int n);

    char* form_log(int& index, char *buf, size_t buf_size, unsigned char *buffer, size_t size);

    void write_thread_proc();

private:
    bool persist_flag_; ///< 是否持久化日志系统。
    FILE* log_file_; ///< 日志文件指针。
    std::string log_name_; ///< 日志文件名。
    std::string log_id_; ///< 日志标识符。
    bool truncate_flag_; ///< 是否截断长日志。
    LOG_LEVEL cur_level_; ///< 当前日志级别。
    uint64_t roll_size_; ///< 日志文件的最大滚动大小。
    uint64_t writen_size_; ///< 已写入日志文件的数据总大小。
    std::list<std::string> write_wating_lists_; ///< 等待写入的日志消息列表。
    std::unique_ptr<std::thread> write_thread_pool_; ///< 异步写日志的线程池。
    std::mutex write_mutex_; ///< 用于同步日志写入的互斥锁。
    std::condition_variable  write_cond_;
    bool exit_flag_;
    bool running_flag_;
}; // whisp_log_h

#endif // TALK_LOG_H
