#pragma once

#include <Obscura/API.hpp>
#include "Asset.hpp"
#include "AssetWorker.hpp"

#include <cppcoro/coroutine.hpp>
#include <cppcoro/task.hpp>
#include <cppcoro/shared_task.hpp>

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Obscura
{
    class OBSCURA_ENGINE_API AssetManager
    {
    public:
        static AssetManager& Get();

        AssetManager();
        ~AssetManager();

        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;
        AssetManager(AssetManager&&) = delete;
        AssetManager& operator=(AssetManager&&) = delete;

        void Initialize(std::filesystem::path rootAssetDirectory = "");
        void Shutdown();

        cppcoro::task<std::shared_ptr<TextureAsset>> LoadTextureAsync(std::filesystem::path path);

        [[nodiscard]] std::shared_ptr<TextureAsset> GetTexture(const std::string& path) const;
        [[nodiscard]] std::shared_ptr<TextureAsset> GetTextureByUUID(std::uint64_t uuid) const;
        [[nodiscard]] std::shared_ptr<Asset> GetAssetByUUID(std::uint64_t uuid) const;

        void RegisterAsset(std::shared_ptr<Asset> asset);
        void UnregisterAsset(std::uint64_t uuid);
        void Clear();

        [[nodiscard]] std::size_t GetAssetCount() const;
        [[nodiscard]] const std::filesystem::path& GetRootDirectory() const noexcept;

    private:
        std::filesystem::path                                              m_RootDirectory;
        mutable std::mutex                                                 m_RegistryMutex;
        std::unordered_map<std::string, std::shared_ptr<Asset>>            m_PathToAsset;
        std::unordered_map<std::uint64_t, std::shared_ptr<Asset>>          m_UUIDToAsset;

        mutable std::mutex                                                 m_InFlightMutex;
        std::unordered_map<std::string, cppcoro::shared_task<std::shared_ptr<TextureAsset>>> m_InFlightTextures;
    };
}
