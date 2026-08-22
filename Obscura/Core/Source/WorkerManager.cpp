#include <Obscura/WorkerManager.hpp>
#include <Obscura/Logger.hpp>

namespace Obscura
{
    WorkerManager& WorkerManager::Get()
    {
        static WorkerManager s_Instance;
        return s_Instance;
    }

    WorkerManager::WorkerManager()
    {
    }

    WorkerManager::~WorkerManager()
    {
        Shutdown();
    }

    void WorkerManager::Initialize(
        std::size_t assetThreads,
        std::size_t generalThreads,
        std::size_t ioThreads)
    {
        std::lock_guard<std::mutex> lock(m_InitMutex);
        if (m_Initialized)
        {
            return;
        }

        const auto hardwareThreads = std::max(1u, std::thread::hardware_concurrency());

        // Default to hardware_concurrency / 2 for Asset workers to protect UI & simulation loop
        if (assetThreads == 0)
        {
            assetThreads = std::max(1u, hardwareThreads / 2);
        }

        if (generalThreads == 0)
        {
            generalThreads = std::max(1u, hardwareThreads / 2);
        }

        LOG_INFO("[WorkerManager] Initializing worker pools: {} Asset workers, {} General workers, {} IO workers (Hardware concurrency: {})",
            assetThreads, generalThreads, ioThreads, hardwareThreads);

        m_AssetPool = std::make_unique<ThreadPool>(assetThreads, "AssetWorker");
        m_GeneralPool = std::make_unique<ThreadPool>(generalThreads, "GeneralWorker");
        m_IOPool = std::make_unique<ThreadPool>(ioThreads, "IOWorker");

        m_Initialized = true;
    }

    void WorkerManager::Shutdown()
    {
        std::lock_guard<std::mutex> lock(m_InitMutex);
        if (!m_Initialized)
        {
            return;
        }

        LOG_INFO("[WorkerManager] Shutting down worker pools...");
        if (m_AssetPool)
        {
            m_AssetPool->Shutdown();
            m_AssetPool.reset();
        }
        if (m_GeneralPool)
        {
            m_GeneralPool->Shutdown();
            m_GeneralPool.reset();
        }
        if (m_IOPool)
        {
            m_IOPool->Shutdown();
            m_IOPool.reset();
        }

        m_Initialized = false;
        LOG_INFO("[WorkerManager] Shutdown complete.");
    }

    ThreadPool& WorkerManager::GetPool(WorkerType type)
    {
        switch (type)
        {
        case WorkerType::Asset:
            return GetAssetPool();
        case WorkerType::IO:
            return GetIOPool();
        case WorkerType::General:
        default:
            return GetGeneralPool();
        }
    }

    ThreadPool& WorkerManager::GetAssetPool()
    {
        if (!m_Initialized)
        {
            Initialize();
        }
        return *m_AssetPool;
    }

    ThreadPool& WorkerManager::GetGeneralPool()
    {
        if (!m_Initialized)
        {
            Initialize();
        }
        return *m_GeneralPool;
    }

    ThreadPool& WorkerManager::GetIOPool()
    {
        if (!m_Initialized)
        {
            Initialize();
        }
        return *m_IOPool;
    }

    bool WorkerManager::IsInitialized() const noexcept
    {
        return m_Initialized;
    }

    std::size_t WorkerManager::GetTotalWorkerCount() const noexcept
    {
        std::size_t total = 0;
        if (m_AssetPool) total += m_AssetPool->GetThreadCount();
        if (m_GeneralPool) total += m_GeneralPool->GetThreadCount();
        if (m_IOPool) total += m_IOPool->GetThreadCount();
        return total;
    }
}
