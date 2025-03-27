#ifndef WHISP_THREAD_POOL_H
#define WHISP_THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <tuple>
#include <stdexcept>
#include "whisp_log.h"

class WhispThreadPool {
public:
    explicit WhispThreadPool(size_t num_threads);
    ~WhispThreadPool();

    // 禁止拷贝和赋值
    WhispThreadPool(const WhispThreadPool&) = delete;
    WhispThreadPool& operator=(const WhispThreadPool&) = delete;

    // C++中，模板函数的定义必须对编译器可见，通常需要将定义放在头文件中，而不是单独的实现文件（.cpp 文件）中
    // 向线程池提交任务
    template <class F, class... Args>
    void enqueue(F&& f, Args&&... args) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (stop) {
                throw std::runtime_error("ThreadPool has stopped!");
            }
    
            // 采用 std::apply 调用任务，并捕获异常
            tasks.emplace([f = std::forward<F>(f), args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                try {
                    std::apply(std::move(f), std::move(args));
                } catch (const std::exception& e) {
                    TALKO_LOG_ERROR("Task execution failed: " , e.what());
                } catch (...) {
                    TALKO_LOG_ERROR("Unknown exception in task execution.");
                }
            });
        }
        condition.notify_one(); // 唤醒一个线程处理任务
    }
    
    // 调整线程池大小
    void resize(size_t num_threads);

private:
    std::vector<std::thread> workers; // 线程集合
    std::queue<std::function<void()>> tasks; // 任务队列

    std::mutex queue_mutex; // 互斥锁
    std::condition_variable condition; // 条件变量
    std::atomic<bool> stop; // 线程池是否停止
};

#endif // WHISP_THREAD_POOL_H
