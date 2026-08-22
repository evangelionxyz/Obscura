#include "AssetManager.hpp"
#include <Obscura/Logger.hpp>
#include <Obscura/VFS.hpp>

namespace Obscura
{
    namespace
    {
        std::uint64_t HashPathToUUID(const std::string& path)
        {
            return std::hash<std::string>{}(path);
        }
    }

    AssetManager& AssetManager::Get()
    {
        static AssetManager s_Instance;
        return s_Instance;
    }

    AssetManager::AssetManager() = default;

    AssetManager::~AssetManager()
    {
        Shutdown();
    }

    void AssetManager::Initialize(std::filesystem::path rootAssetDirectory)
    {
        if (rootAssetDirectory.empty())
        {
            m_RootDirectory = VFS::GetModuleDirectory() / "Resources";
        }
        else
        {
            m_RootDirectory = std::move(rootAssetDirectory);
        }

        LOG_INFO("[AssetManager] Initialized with root directory: '{}'", m_RootDirectory.string());
    }

    void AssetManager::Shutdown()
    {
        Clear();
        LOG_INFO("[AssetManager] Shutdown complete.");
    }

    cppcoro::task<std::shared_ptr<TextureAsset>> AssetManager::LoadTextureAsync(std::filesystem::path path)
    {
        std::filesystem::path fullPath = path;
        if (fullPath.is_relative() && !m_RootDirectory.empty())
        {
            fullPath = m_RootDirectory / path;
        }

        const std::string fullPathStr = fullPath.lexically_normal().string();

        // 1. Check if already cached
        {
            std::lock_guard<std::mutex> lock(m_RegistryMutex);
            auto it = m_PathToAsset.find(fullPathStr);
            if (it != m_PathToAsset.end())
            {
                if (auto texture = std::dynamic_pointer_cast<TextureAsset>(it->second))
                {
                    co_return texture;
                }
            }
        }

        // 2. Perform or await in-flight decoding
        std::uint64_t uuid = HashPathToUUID(fullPathStr);
        auto textureAsset = co_await AssetWorker::LoadTextureAssetAsync(fullPath, uuid);

        if (textureAsset && textureAsset->GetState() == AssetState::Ready)
        {
            RegisterAsset(textureAsset);
        }
        else
        {
            LOG_WARN("[AssetManager] Loaded texture '{}' state is not Ready.", fullPathStr);
        }

        co_return textureAsset;
    }

    std::shared_ptr<TextureAsset> AssetManager::GetTexture(const std::string& path) const
    {
        std::lock_guard<std::mutex> lock(m_RegistryMutex);
        auto it = m_PathToAsset.find(path);
        if (it != m_PathToAsset.end())
        {
            return std::dynamic_pointer_cast<TextureAsset>(it->second);
        }
        return nullptr;
    }

    std::shared_ptr<TextureAsset> AssetManager::GetTextureByUUID(std::uint64_t uuid) const
    {
        std::lock_guard<std::mutex> lock(m_RegistryMutex);
        auto it = m_UUIDToAsset.find(uuid);
        if (it != m_UUIDToAsset.end())
        {
            return std::dynamic_pointer_cast<TextureAsset>(it->second);
        }
        return nullptr;
    }

    std::shared_ptr<Asset> AssetManager::GetAssetByUUID(std::uint64_t uuid) const
    {
        std::lock_guard<std::mutex> lock(m_RegistryMutex);
        auto it = m_UUIDToAsset.find(uuid);
        if (it != m_UUIDToAsset.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void AssetManager::RegisterAsset(std::shared_ptr<Asset> asset)
    {
        if (!asset) return;

        std::lock_guard<std::mutex> lock(m_RegistryMutex);
        m_PathToAsset[asset->GetPath()] = asset;
        m_UUIDToAsset[asset->GetUUID()] = asset;
    }

    void AssetManager::UnregisterAsset(std::uint64_t uuid)
    {
        std::lock_guard<std::mutex> lock(m_RegistryMutex);
        auto it = m_UUIDToAsset.find(uuid);
        if (it != m_UUIDToAsset.end())
        {
            m_PathToAsset.erase(it->second->GetPath());
            m_UUIDToAsset.erase(it);
        }
    }

    void AssetManager::Clear()
    {
        std::lock_guard<std::mutex> lock(m_RegistryMutex);
        m_PathToAsset.clear();
        m_UUIDToAsset.clear();
    }

    std::size_t AssetManager::GetAssetCount() const
    {
        std::lock_guard<std::mutex> lock(m_RegistryMutex);
        return m_UUIDToAsset.size();
    }

    const std::filesystem::path& AssetManager::GetRootDirectory() const noexcept
    {
        return m_RootDirectory;
    }
}
