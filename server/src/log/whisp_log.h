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
#define WHISP_LOG_TRACE(...)      WhispLog::output(LOG_LEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define WHISP_LOG_DEBUG(...)      WhispLog::output(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define WHISP_LOG_INFO(...)       WhispLog::output(LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define WHISP_LOG_WARNING(...)    WhispLog::output(LOG_LEVEL_WARNING, __FILE__, __LINE__,__VA_ARGS__)
#define WHISP_LOG_ERROR(...)      WhispLog::output(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define WHISP_LOG_SYSERROR(...)   WhispLog::output(LOG_LEVEL_SYSERROR, __FILE__, __LINE__, __VA_ARGS__)
#define WHISP_LOG_FALTAL(...)     WhispLog::output(LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)        //为了让FATAL级别的日志能立即crash程序，采取同步写日志的方法
#define WHISP_LOG_CRITICAL(...)   WhispLog::output(LOG_LEVEL_CRITICAL, __FILE__, __LINE__, __VA_ARGS__)     //关键信息，无视日志级别，总是输出

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
    /**
     * @brief 初始化日志系统。
     * 
     * @param log_file_name 日志文件名。如果为 nullptr，则不进行文件日志记录。
     * @param truncate_flag 是否截断长日志行。
     * @param roll_size 日志文件的最大滚动大小（默认：10 MB）。
     * @return 如果初始化成功返回 true，否则返回 false。
     */
    bool log_init(const char* log_file_name = nullptr, bool truncate_flag = false, int64_t roll_size = 10 * 1024 * 1024);

    /**
     * @brief 反初始化日志系统并释放资源。
     */
    void log_uninit();

    /**
     * @brief 设置日志级别。
     * 
     * @param levelv 目标日志级别。
     */
    void log_set_level(LOG_LEVEL levelv);

    /**
     * @brief 检查日志系统是否正在运行。
     * 
     * @return 如果日志系统正在运行返回 true，否则返回 false。
     */
    bool log_isrunning();

    /**
     * @brief 输出日志消息（不包含线程 ID、函数签名或行号）。
     * 
     * @param nLevel 日志消息的级别。
     * @param pszFmt 日志消息的格式化字符串。
     * @param ... 格式化字符串的附加参数。
     * @return 如果日志消息成功输出返回 true，否则返回 false。
     */
    bool log_output(LOG_LEVEL levelv, const char* fmt, ...);

    /**
     * @brief 输出日志消息（包含线程 ID、函数签名和行号）。
     * 
     * @param nLevel 日志消息的级别。
     * @param pszFileName 源文件名。
     * @param nLineNo 源文件中的行号。
     * @param pszFmt 日志消息的格式化字符串。
     * @param ... 格式化字符串的附加参数。
     * @return 如果日志消息成功输出返回 true，否则返回 false。
     */
    bool log_output(LOG_LEVEL levelv, const char* file_name, int line_num, const char* fmt, ...);

    /**
     * @brief 输出二进制数据到日志。
     * 
     * @param buffer 二进制数据缓冲区。
     * @param size 二进制数据大小。
     * @return 如果二进制数据成功输出返回 true，否则返回 false。
     */
    bool log_output_binary(unsigned char* buffer, size_t size);

private:
    /**
     * @brief 根据日志级别设置日志行的前缀。
     * 
     * @param levelv 日志级别。
     * @param prefix_str 用于存储生成的前缀字符串。
     */
    void set_line_perfix(LOG_LEVEL levelv, std::string& prefix_str);

    /**
     * @brief 获取当前时间戳字符串。
     * 
     * @param ts 用于存储时间戳的缓冲区。
     * @param ts_len 缓冲区的长度。
     */
    void get_time(char* ts, int ts_len);

    /**
     * @brief 创建指定名称的日志文件。
     * 
     * @param log_file_name 日志文件名。
     * @return 如果文件成功创建返回 true，否则返回 false。
     */
    bool create_file(const char* log_file_name);

    /**
     * @brief 写入数据到日志文件。
     * 
     * @param data 要写入的数据。
     * @return 如果数据成功写入返回 true，否则返回 false。
     */
    bool write2file(const std::string& data);

    /**
     * @brief 强制使程序崩溃。
     */
    void crash();

    /**
     * @brief 将整数转换为字符串。
     * 
     * @param n 要转换的整数。
     * @return 整数的字符串表示形式的指针。
     */
    const char* ull2str(int n);

    /**
     * @brief 格式化包含二进制数据的日志消息。
     * 
     * @param index 用于格式化的索引。
     * @param buf 用于存储格式化日志消息的缓冲区。
     * @param buf_size 缓冲区大小。
     * @param buffer 二进制数据缓冲区。
     * @param size 二进制数据大小。
     * @return 格式化日志消息的指针。
     */
    char* form_log(int& index, char *buf, size_t buf_size, unsigned char *buffer, size_t size);

    /**
     * @brief 异步写日志的线程过程。
     */
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
