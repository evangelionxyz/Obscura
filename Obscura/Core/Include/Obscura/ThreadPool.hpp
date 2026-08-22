#pragma once

#include <cppcoro/coroutine.hpp>
#include <cppcoro/task.hpp>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

namespace Obscura
{
    class ThreadPool;

    struct ScheduleAwaiter
    {
        ThreadPool* m_Pool = nullptr;

        explicit ScheduleAwaiter(ThreadPool* pool) noexcept : m_Pool(pool) {}

        [[nodiscard]] bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> handle) const noexcept;
        void await_resume() const noexcept {}
    };

    class ThreadPool
    {
    public:
        explicit ThreadPool(
            std::size_t threadCount = 0,
            std::string poolName = "Worker");
        ~ThreadPool();

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        template <typename F, typename... Args>
        auto Enqueue(F&& f, Args&&... args) 
            -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>
        {
            using return_type = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

            auto task = std::make_shared<std::packaged_task<return_type()>>(
                [func = std::forward<F>(f), ...capturedArgs = std::forward<Args>(args)]() mutable {
                    return std::invoke(std::forward<F>(func), std::forward<Args>(capturedArgs)...);
                }
            );

            std::future<return_type> res = task->get_future();
            EnqueueTask([task]() { (*task)(); });
            return res;
        }

        void EnqueueTask(std::function<void()> task);

        [[nodiscard]] ScheduleAwaiter Schedule() noexcept;

        void Shutdown();
        void WaitIdle();

        [[nodiscard]] std::size_t GetThreadCount() const noexcept;
        [[nodiscard]] std::size_t GetPendingTaskCount() const noexcept;
        [[nodiscard]] bool IsRunning() const noexcept;
        [[nodiscard]] const std::string& GetName() const noexcept;

    private:
        void WorkerLoop(std::size_t workerIndex);

    private:
        std::string                       m_PoolName;
        std::vector<std::thread>          m_Workers;
        std::queue<std::function<void()>> m_Tasks;

        mutable std::mutex                m_QueueMutex;
        std::condition_variable           m_Cv;
        std::condition_variable           m_IdleCv;

        std::atomic<bool>                 m_Stop;
        std::atomic<std::size_t>          m_ActiveTasks;
    };
}
