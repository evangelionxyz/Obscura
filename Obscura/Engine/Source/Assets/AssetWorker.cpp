#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "AssetWorker.hpp"
#include <Obscura/Logger.hpp>
#include <Obscura/WorkerManager.hpp>

#include <chrono>
#include <fstream>

namespace Obscura
{
    cppcoro::task<TextureData> AssetWorker::LoadTextureDataAsync(std::filesystem::path filepath)
    {
        // Hop to background Asset Worker ThreadPool (runs on hardware_concurrency / 2 threads)
        co_await WorkerManager::Get().GetAssetPool().Schedule();

        TextureData result;
        const std::string pathStr = filepath.string();

        if (!std::filesystem::exists(filepath))
        {
            LOG_WARN("[AssetWorker] File does not exist: '{}'", pathStr);
            co_return result;
        }

        int width = 0;
        int height = 0;
        int channels = 0;

        // Force 4 channels (RGBA) for standard engine texture alignment
        stbi_uc* pixels = stbi_load(pathStr.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels)
        {
            LOG_ERROR("[AssetWorker] Failed to decode image '{}': {}", pathStr, stbi_failure_reason());
            co_return result;
        }

        result.width = static_cast<std::uint32_t>(width);
        result.height = static_cast<std::uint32_t>(height);
        result.channels = 4;
        result.isValid = true;

        const std::size_t byteSize = static_cast<std::size_t>(width * height * 4);
        result.pixels.resize(byteSize);
        std::memcpy(result.pixels.data(), pixels, byteSize);

        stbi_image_free(pixels);

        LOG_INFO("[AssetWorker] Decoded texture '{}' ({}x{}, 4ch, {} KB) on background worker thread.",
            pathStr, width, height, byteSize / 1024);

        co_return result;
    }

    cppcoro::task<std::shared_ptr<TextureAsset>> AssetWorker::LoadTextureAssetAsync(
        std::filesystem::path filepath,
        std::uint64_t uuid)
    {
        auto textureAsset = std::make_shared<TextureAsset>(uuid, filepath.string());
        textureAsset->SetState(AssetState::Loading);

        TextureData data = co_await LoadTextureDataAsync(filepath);
        textureAsset->SetData(std::move(data));

        co_return textureAsset;
    }
}
