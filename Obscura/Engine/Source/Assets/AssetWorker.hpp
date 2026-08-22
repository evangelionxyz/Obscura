#pragma once

#include <Obscura/API.hpp>
#include "Asset.hpp"

#include <cppcoro/coroutine.hpp>
#include <cppcoro/task.hpp>

#include <filesystem>
#include <memory>

namespace Obscura
{
    class OBSCURA_ENGINE_API AssetWorker
    {
    public:
        static cppcoro::task<TextureData> LoadTextureDataAsync(std::filesystem::path filepath);
        static cppcoro::task<std::shared_ptr<TextureAsset>> LoadTextureAssetAsync(
            std::filesystem::path filepath,
            std::uint64_t uuid = 0);
    };
}
