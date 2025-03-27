#include "whisp_thread_pool.h"
// #include "TalkLog.h"

// 构造函数：启动线程
WhispThreadPool::WhispThreadPool(size_t num_threads) : stop(false) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    condition.wait(lock, [this] { return stop || !tasks.empty(); });

                    if (stop && tasks.empty())
                        return; // 线程退出

                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task(); // 执行任务
            }
        });
    }
}

// 线程池析构：等待所有线程结束
WhispThreadPool::~WhispThreadPool() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        stop = true;
    }

    condition.notify_all(); // 唤醒所有线程

    for (std::thread &worker : workers) {
        if (worker.joinable()) {
            worker.join(); // 等待线程完成
        }
    }
}

// 提交任务
// template <class F, class... Args>
// void TalkThreadPool::enqueue(F&& f, Args&&... args) {
//     {
//         std::lock_guard<std::mutex> lock(queue_mutex);
//         if (stop) {
//             throw std::runtime_error("ThreadPool has stopped!");
//         }

//         // 采用 std::apply 调用任务，并捕获异常
//         tasks.emplace([f = std::forward<F>(f), args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
//             try {
//                 std::apply(std::move(f), std::move(args));
//             } catch (const std::exception& e) {
//                 TALKO_LOG_ERROR("Task execution failed: " , e.what());
//             } catch (...) {
//                 TALKO_LOG_ERROR("Unknown exception in task execution.");
//             }
//         });
//     }
//     condition.notify_one(); // 唤醒一个线程处理任务
// }

// 调整线程池大小
void WhispThreadPool::resize(size_t num_threads) {
    if (num_threads < workers.size()) {
        // 减少线程数量
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (size_t i = 0; i < workers.size() - num_threads; ++i) {
            if (workers[i].joinable()) {
                workers[i].join();
            }
        }
        workers.resize(num_threads);
        stop = false;
    } else if (num_threads > workers.size()) {
        // 增加线程数量
        for (size_t i = workers.size(); i < num_threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });

                        if (stop && tasks.empty())
                            return; // 线程退出

                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task(); // 执行任务
                }
            });
        }
    }
}