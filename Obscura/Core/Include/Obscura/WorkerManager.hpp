#pragma once

#include <Obscura/ThreadPool.hpp>

#include <memory>
#include <mutex>
#include <thread>

namespace Obscura
{
    enum class WorkerType
    {
        Asset,
        General,
        IO
    };

    class WorkerManager
    {
    public:
        static WorkerManager& Get();

        WorkerManager();
        ~WorkerManager();

        WorkerManager(const WorkerManager&) = delete;
        WorkerManager& operator=(const WorkerManager&) = delete;
        WorkerManager(WorkerManager&&) = delete;
        WorkerManager& operator=(WorkerManager&&) = delete;

        void Initialize(
            std::size_t assetThreads = 0,
            std::size_t generalThreads = 0,
            std::size_t ioThreads = 2);

        void Shutdown();

        [[nodiscard]] ThreadPool& GetPool(WorkerType type);
        [[nodiscard]] ThreadPool& GetAssetPool();
        [[nodiscard]] ThreadPool& GetGeneralPool();
        [[nodiscard]] ThreadPool& GetIOPool();

        [[nodiscard]] bool IsInitialized() const noexcept;
        [[nodiscard]] std::size_t GetTotalWorkerCount() const noexcept;

    private:
        std::unique_ptr<ThreadPool> m_AssetPool;
        std::unique_ptr<ThreadPool> m_GeneralPool;
        std::unique_ptr<ThreadPool> m_IOPool;

        std::mutex                  m_InitMutex;
        bool                        m_Initialized = false;
    };
}
