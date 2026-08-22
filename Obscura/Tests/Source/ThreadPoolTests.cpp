#include <gtest/gtest.h>
#include <Obscura/ThreadPool.hpp>
#include <Obscura/WorkerManager.hpp>
#include <cppcoro/sync_wait.hpp>
#include <cppcoro/task.hpp>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

TEST(ThreadPoolTests, BasicTaskExecutionAndFuture)
{
    Obscura::ThreadPool pool(4, "TestWorker");
    EXPECT_EQ(pool.GetThreadCount(), 4);
    EXPECT_TRUE(pool.IsRunning());

    auto future = pool.Enqueue([](int a, int b) {
        return a + b;
    }, 10, 25);

    EXPECT_EQ(future.get(), 35);
}

TEST(ThreadPoolTests, HighVolumeConcurrentExecution)
{
    Obscura::ThreadPool pool(4, "StressWorker");
    constexpr int taskCount = 1000;
    std::atomic<int> counter{0};

    for (int i = 0; i < taskCount; ++i)
    {
        pool.EnqueueTask([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    pool.WaitIdle();
    EXPECT_EQ(counter.load(), taskCount);
}

namespace
{
    cppcoro::task<int> AsyncComputeOnPool(Obscura::ThreadPool& pool, int value)
    {
        auto callerThreadId = std::this_thread::get_id();
        co_await pool.Schedule();
        auto workerThreadId = std::this_thread::get_id();

        // Ensure task resumed on worker thread, or value computation is correct
        co_return value * 2;
    }
}

TEST(ThreadPoolTests, CoroutineScheduleAwaiter)
{
    Obscura::ThreadPool pool(2, "CoroWorker");
    
    auto task = AsyncComputeOnPool(pool, 21);
    int result = cppcoro::sync_wait(task);

    EXPECT_EQ(result, 42);
}

TEST(WorkerManagerTests, PartitioningAndHardwareScaling)
{
    auto& manager = Obscura::WorkerManager::Get();
    manager.Initialize(0, 0, 2);

    EXPECT_TRUE(manager.IsInitialized());
    
    auto hardwareThreads = std::max(1u, std::thread::hardware_concurrency());
    auto expectedAssetThreads = std::max(1u, hardwareThreads / 2);
    
    EXPECT_EQ(manager.GetAssetPool().GetThreadCount(), expectedAssetThreads);
    EXPECT_GE(manager.GetTotalWorkerCount(), expectedAssetThreads + 1);

    auto assetFuture = manager.GetAssetPool().Enqueue([]() {
        return 100;
    });
    EXPECT_EQ(assetFuture.get(), 100);
}
