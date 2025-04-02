#include "whisp_log.h"
// #include <ctime>
// #include <time.h>
// #include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <stdarg.h>
#include <stdarg.h>
#include <sstream>
#include <string.h>
#include <sys/timeb.h>

#define MAX_LOG_LINE_LENGTH     (256)
#define DEFAULT_ROLL_SIZE       (10*1024*1024)

bool WhispLog::log_init(const char* log_file_name = nullptr, bool truncate_flag = false, int64_t roll_size = 10 * 1024 * 1024)
{
    truncate_flag_ = truncate_flag;
    roll_size_ = roll_size;

    if (nullptr == log_file_name || log_file_name[0] == 0) {
        log_name_.clear();
    } else {
        log_name_ = log_file_name;
    }

    // 获取进程ID
    char pid[8];
    snprintf(pid, sizeof(pid), "%05d", (int)getpid());
    log_id_ = pid;
    
    log_file_ = nullptr;
    cur_level_ = LOG_LEVEL_INFO;
    writen_size_ = 0;
    exit_flag_ = false;
    running_flag_ = false;

    write_thread_pool_.reset(new std::thread(write_thread_proc));

    return true;
}

void WhispLog::log_uninit()
{
    running_flag_ = false;
    exit_flag_ = true;
    write_cond_.notify_one();

    if (write_thread_pool_->joinable())
        write_thread_pool_->join();

    if (nullptr != log_file_) {
        fclose(log_file_);
        log_file_ = nullptr;
    }
}

void WhispLog::log_set_level(LOG_LEVEL levelv)
{
    if (levelv < LOG_LEVEL_TRACE || levelv > LOG_LEVEL_FATAL) {
        std::cout << "set log level error(%d)" << levelv << std::endl;
        return;
    }
    cur_level_ = levelv;

    return;
}

bool WhispLog::log_isrunning()
{
    return running_flag_;
}

bool WhispLog::log_output(LOG_LEVEL levelv, const char* fmt, ...)
{
    if (levelv < cur_level_ && levelv != LOG_LEVEL_CRITICAL)
        return false;

    std::string output_line;
    set_line_perfix(levelv, output_line);

    // log context
    std::string log_msg;
    // 计算不定参数长度，以便分配空间
    va_list ap;     // #include <stdarg.h>
    va_start(ap, fmt);
    int msg_len = vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);

    // string 内容正确但len错误
    std::string formal_msg;
    formal_msg.append(log_msg.c_str(), msg_len);

    if (truncate_flag_)
        formal_msg = formal_msg.substr(0, MAX_LOG_LINE_LENGTH);
    
    output_line += formal_msg;

    if (!log_name_.empty()){
        output_line += '\n';
    } 
    
    if (levelv != LOG_LEVEL_FATAL) {
        std::lock_guard<std::mutex> lock_guard(write_mutex_);
        write_wating_lists_.push_back(output_line);
        write_cond_.notify_one();
    } else {
        //为了让FATAL级别的日志能立即crash程序，采取同步写日志的方法
        std::cout << output_line << std::endl;

        if (!log_name_.empty()) {
            if (nullptr == log_file_) {
                // 没有文件句柄，新建文件
                char timef[64];
                time_t cur_time = time(NULL);
                tm time_info;

                localtime_r(&cur_time, &time_info);
                strftime(timef, sizeof(timef), "%Y%m%d%H%M%S", &time_info);

                std::string new_log_file(log_name_);
                new_log_file += ".";
                new_log_file += timef;
                new_log_file += ".";
                new_log_file += log_id_;
                new_log_file += ".log";
                if (!create_file(new_log_file.c_str())) {
                    std:: cout << "creat log file :" << new_log_file << " fail" << std::endl;
                    return false;
                }
            }
            write2file(output_line);
        }
        crash();
    }
    return true;
}

bool WhispLog::log_output(LOG_LEVEL levelv, const char* file_name, int line_num, const char* fmt, ...)
{
    if (levelv < cur_level_ || levelv != LOG_LEVEL_CRITICAL)
        return false;

    std::string out_line;
    set_line_perfix(levelv, out_line);

    // 函数签名
    char file_info[512] = {0};
    snprintf(file_info, sizeof(file_info), "[%s:%d]", file_name, line_num);
    out_line += file_info;

    // log msg
    std::string log_msg;
    va_list ap;
    va_start(ap, fmt);
    int msg_len = vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);

    // + \0
    if ((int)log_msg.capacity() < msg_len + 1) {
        log_msg.resize(msg_len + 1);
    }
    va_list aq;
    va_start(aq, fmt);
    vsnprintf((char*)log_msg.data(), log_msg.capacity(), fmt, aq);
    va_end(aq);

    std::string formal_msg;
    formal_msg.append(log_msg.c_str(), msg_len); // 是否少1？

    if (truncate_flag_)
        formal_msg = formal_msg.substr(0, MAX_LOG_LINE_LENGTH); // 是否需要'\0'

    out_line += formal_msg;

    if (! log_name_.empty()) {
        out_line += '\n';
    }

    if (levelv != LOG_LEVEL_FATAL) {
        std::lock_guard<std::mutex> lock_guard(write_mutex_);
        write_wating_lists_.push_back(out_line);
        write_cond_.notify_one();
    } else {
        //为了让FATAL级别的日志能立即crash程序，采取同步写日志的方法
        std::cout << out_line << std::endl;

        if (!log_name_.empty()) {
            if (nullptr == log_file_) {
                // 没有文件句柄，新建文件
                char timef[64];
                time_t cur_time = time(NULL);
                tm time_info;

                localtime_r(&cur_time, &time_info);
                strftime(timef, sizeof(timef), "%Y%m%d%H%M%S", &time_info);

                std::string new_log_file(log_name_);
                new_log_file += ".";
                new_log_file += timef;
                new_log_file += ".";
                new_log_file += log_id_;
                new_log_file += ".log";
                if (!create_file(new_log_file.c_str())) {
                    std:: cout << "creat log file :" << new_log_file << " fail" << std::endl;
                    return false;
                }
            }
            write2file(out_line);
        }
        crash();
    }
}

bool WhispLog::log_output_binary(unsigned char* buffer, size_t size)
{
    std::ostringstream os;  // #include <sstream>
    
    static const size_t PRINT_SIZE = 512;
    char tmpbuf[PRINT_SIZE * 3 + 8];
    size_t lsize = 0, lprintbuf_size = 0;
    int index = 0;
    os << "address[" << (long)buffer << "] size[" << size << "] \n";

    while (true) {
        memset(tmpbuf, 0, sizeof(tmpbuf));
        
        if (size > lsize) {
            lprintbuf_size = (size - lsize) > PRINT_SIZE ? PRINT_SIZE : (size - lsize);
            form_log(index, tmpbuf, sizeof(tmpbuf), buffer + lsize, lprintbuf_size);
            size_t len = strlen(tmpbuf);

            os << tmpbuf;
            lsize += lprintbuf_size;
        } else {
            break;
        }
    }

    std::lock_guard<std::mutex> lock_guard(write_mutex_);
    write_wating_lists_.push_back(os.str());
    write_cond_.notify_one();

    return true;
}

const char *WhispLog::ull2str(int n)
{
    char tmpbuf[64 + 1];    // static ?
    memset(tmpbuf, 0, sizeof(tmpbuf));
    sprintf(tmpbuf, "%06u", n);
    return tmpbuf;
}

char *WhispLog::form_log(int& index, char *buf, size_t buf_size, unsigned char *buffer, size_t size)
{
    size_t len = 0;
    size_t lsize = 0;
    int head_len = 0;
    char head_buf[64 + 1] = {0};
    char magic_num[17] = "0123456789abcdef";

    while (size > lsize && len + 10 < buf_size) {
        if (lsize % 32 == 0) {
            if (0 != head_len) {
                buf[len++] = '\n';
            }
            memset(head_buf, 0, sizeof(head_buf));
            strncpy(head_buf, ull2str(index++), sizeof(head_buf) - 1);
            head_len = strlen(head_buf);
            head_buf[head_len++] = ' ';
            
            strcat(buf, head_buf);
            len += head_len;
        }
        if (lsize % 16 == 0 && 0 != head_len) {
            buf[len++] = ' ';
        }
        buf[len++] = magic_num[(buffer[lsize] >> 4) & 0xf];
        buf[len++] = magic_num[(buffer[lsize]) & 0xf];
        lsize++;
    }
    buf[len++] = '\n';
    buf[len++] = '\0';

    return buf;
}

void WhispLog::set_line_perfix(LOG_LEVEL levelv, std::string& prefix_str)
{
    //级别
    prefix_str = "[INFO]";
    if (levelv == LOG_LEVEL_TRACE)
        prefix_str = "[TRACE]";
    else if (levelv == LOG_LEVEL_DEBUG)
        prefix_str = "[DEBUG]";
    else if (levelv == LOG_LEVEL_WARNING)
        prefix_str = "[WARN]";
    else if (levelv == LOG_LEVEL_ERROR)
        prefix_str = "[ERROR]";
    else if (levelv == LOG_LEVEL_SYSERROR)
        prefix_str = "[SYSE]";
    else if (levelv == LOG_LEVEL_FATAL)
        prefix_str = "[FATAL]";
    else if (levelv == LOG_LEVEL_CRITICAL)
        prefix_str = "[CRITICAL]";

    //时间
    char szTime[64] = { 0 };
    get_time(szTime, sizeof(szTime));

    prefix_str += "[";
    prefix_str += szTime;
    prefix_str += "]";

    //当前线程信息
    char thread_id[32] = { 0 };
    std::ostringstream osThreadID;
    osThreadID << std::this_thread::get_id();
    snprintf(thread_id, sizeof(thread_id), "[%s]", osThreadID.str().c_str());
    prefix_str += thread_id;   
}

void WhispLog::get_time(char* ts, int ts_len)
{
    struct timeb tp;
    ftime(&tp);

    time_t now = tp.time;
    tm time;
    localtime_r(&now, &time);

    snprintf(ts, ts_len, "[%04d-%02d-%02d %02d:%02d:%02d:%03d]", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec, tp.millitm);
}

bool WhispLog::create_file(const char *log_file_name)
{
    if (nullptr != log_file_) {
        fclose(log_file_);
    }

    log_file_ = fopen(log_file_name, "w+");
    return nullptr != log_file_;
}

bool WhispLog::write2file(const std::string& data)
{
    std::string tmp(data);
    int ret = 0;
    while (true) {
        ret = fwrite(tmp.c_str(), 1, tmp.length(), log_file_);
        if (ret < 0) {
            return false;
        }
        else if (ret <= (int)tmp.length()) {
            tmp.erase(0, ret); // ?
        }
        
        if (tmp.empty())
            break;
    }
    fflush(log_file_);

    return true;
}

void WhispLog::crash()  // dump掉
{
    char *p = nullptr;
    *p = 0;
}

void WhispLog::write_thread_proc()
{
    running_flag_ = true;

    while (true) {
        if (!log_name_.empty()) {
            if (nullptr == log_file_ || writen_size_ >= roll_size_) {
                writen_size_ = 0;

                // 第一次或者文件大小超过roll size，均新建文件
                char timef[64];
                time_t cur_time = time(NULL);
                tm time_info;

                localtime_r(&cur_time, &time_info);
                strftime(timef, sizeof(timef), "%Y%m%d%H%M%S", &time_info);

                std::string new_log_file(log_name_);
                new_log_file += ".";
                new_log_file += timef;
                new_log_file += ".";
                new_log_file += log_id_;
                new_log_file += ".log";
                if (!create_file(new_log_file.c_str())) {
                    std:: cout << "creat log file :" << new_log_file << " fail" << std::endl;
                    return;
                }                
            }            
        }

        std::string out_line;
        std::unique_lock<std::mutex>guard(write_mutex_);
        while (write_wating_lists_.empty()) {
            if (exit_flag_)
                return;

            write_cond_.wait(guard);
        }

        out_line = write_wating_lists_.front();
        write_wating_lists_.pop_front();

        std::cout << out_line << std::endl;

        if (!log_name_.empty()) {
            if (!write2file(out_line))
                return;

            writen_size_ += out_line.length();
        }
    }

    running_flag_ = false;
}