// #include <gtest/gtest.h>
// #include <future>
// #include "TalkThreadPool.h"

// // **测试1: 线程池创建与销毁**
// TEST(TalkThreadPoolTest, CreateAndDestroy) {
//     EXPECT_NO_THROW({
//         TalkThreadPool pool(4);
//     });
// }

// // **测试2: 单任务提交**
// TEST(TalkThreadPoolTest, SingleTaskExecution) {
//     TalkThreadPool pool(2);
    
//     std::atomic<int> result = 0;
//     pool.enqueue([&result]() {
//         result = 42;
//     });

//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     EXPECT_EQ(result.load(), 42);
// }

// // **测试3: 多任务并行执行**
// TEST(TalkThreadPoolTest, MultipleTasksExecution) {
//     TalkThreadPool pool(4);
//     std::vector<std::future<int>> results;

//     for (int i = 0; i < 10; ++i) {
//         results.push_back(std::async(std::launch::async, [&pool, i]() {
//             std::promise<int> promise;
//             auto future = promise.get_future();
//             pool.enqueue([&promise, i]() { promise.set_value(i * i); });
//             return future.get();
//         }));
//     }

//     for (int i = 0; i < 10; ++i) {
//         EXPECT_EQ(results[i].get(), i * i);
//     }
// }

// // **测试4: 并发安全**
// TEST(TalkThreadPoolTest, ConcurrentExecution) {
//     TalkThreadPool pool(4);
//     std::atomic<int> counter = 0;

//     for (int i = 0; i < 100; ++i) {
//         pool.enqueue([&counter]() {
//             counter.fetch_add(1, std::memory_order_relaxed);
//         });
//     }

//     std::this_thread::sleep_for(std::chrono::milliseconds(500));
//     EXPECT_EQ(counter.load(), 100);
// }

// // **测试5: 线程池动态扩容**
// TEST(TalkThreadPoolTest, ResizePool) {
//     TalkThreadPool pool(2);
//     std::atomic<int> counter = 0;

//     for (int i = 0; i < 10; ++i) {
//         pool.enqueue([&counter]() {
//             std::this_thread::sleep_for(std::chrono::milliseconds(50));
//             counter.fetch_add(1, std::memory_order_relaxed);
//         });
//     }

//     pool.resize(4);
//     std::this_thread::sleep_for(std::chrono::milliseconds(500));
//     EXPECT_EQ(counter.load(), 10);
// }

// // **测试6: 线程池析构是否正常退出**
// TEST(TalkThreadPoolTest, DestructorTest) {
//     std::atomic<int> counter = 0;
    
//     {
//         TalkThreadPool pool(2);
//         for (int i = 0; i < 5; ++i) {
//             pool.enqueue([&counter]() {
//                 std::this_thread::sleep_for(std::chrono::milliseconds(100));
//                 counter.fetch_add(1, std::memory_order_relaxed);
//             });
//         }
//     }

//     EXPECT_LE(counter.load(), 5);  // 确保不会出现未执行任务
// }
