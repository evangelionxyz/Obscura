#include <gtest/gtest.h>
#include <Assets/Asset.hpp>
#include <Assets/AssetWorker.hpp>
#include <Assets/AssetManager.hpp>
#include <Obscura/WorkerManager.hpp>

#include <cppcoro/sync_wait.hpp>
#include <cppcoro/task.hpp>

#include <filesystem>
#include <fstream>

namespace
{
    void CreateTestTGAImage(const std::filesystem::path& path, int width, int height)
    {
        // Simple uncompressed 24-bit/32-bit TGA header + pixels for stb_image
        std::ofstream file(path, std::ios::binary);
        unsigned char header[18] = {0};
        header[2] = 2; // uncompressed RGB
        header[12] = width & 0xFF;
        header[13] = (width >> 8) & 0xFF;
        header[14] = height & 0xFF;
        header[15] = (height >> 8) & 0xFF;
        header[16] = 32; // 32 bits per pixel
        header[17] = 0x20; // top-down
        file.write(reinterpret_cast<const char*>(header), 18);

        std::vector<unsigned char> pixels(width * height * 4, 255);
        // RGBA pattern
        for (int i = 0; i < width * height; ++i)
        {
            pixels[i * 4 + 0] = static_cast<unsigned char>((i * 13) % 255); // B
            pixels[i * 4 + 1] = static_cast<unsigned char>((i * 17) % 255); // G
            pixels[i * 4 + 2] = static_cast<unsigned char>((i * 23) % 255); // R
            pixels[i * 4 + 3] = 255; // A
        }
        file.write(reinterpret_cast<const char*>(pixels.data()), pixels.size());
    }
}

TEST(AssetManagerTests, NonExistentTextureFallback)
{
    Obscura::WorkerManager::Get().Initialize();

    auto task = Obscura::AssetWorker::LoadTextureAssetAsync("invalid/path/nonexistent.png");
    auto asset = cppcoro::sync_wait(task);

    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->GetState(), Obscura::AssetState::Failed);
    EXPECT_FALSE(asset->GetData().isValid);
}

TEST(AssetManagerTests, AsyncTextureDecodeAndRegistry)
{
    Obscura::WorkerManager::Get().Initialize();

    auto testPath = std::filesystem::temp_directory_path() / "obscura_test_texture.tga";
    CreateTestTGAImage(testPath, 64, 64);

    auto& manager = Obscura::AssetManager::Get();
    manager.Initialize(std::filesystem::temp_directory_path());

    auto loadTask = manager.LoadTextureAsync(testPath);
    auto texture = cppcoro::sync_wait(loadTask);

    ASSERT_NE(texture, nullptr);
    EXPECT_EQ(texture->GetState(), Obscura::AssetState::Ready);
    EXPECT_TRUE(texture->GetData().isValid);
    EXPECT_EQ(texture->GetData().width, 64);
    EXPECT_EQ(texture->GetData().height, 64);
    EXPECT_EQ(texture->GetData().channels, 4);

    // Verify cached in manager
    auto cached = manager.GetTexture(testPath.lexically_normal().string());
    EXPECT_EQ(cached, texture);

    // Clean up
    std::filesystem::remove(testPath);
}

TEST(AssetManagerTests, ConcurrentTextureLoadDeduplication)
{
    Obscura::WorkerManager::Get().Initialize();

    auto testPath = std::filesystem::temp_directory_path() / "obscura_concurrent_texture.tga";
    CreateTestTGAImage(testPath, 32, 32);

    auto& manager = Obscura::AssetManager::Get();

    auto task1 = manager.LoadTextureAsync(testPath);
    auto task2 = manager.LoadTextureAsync(testPath);

    auto tex1 = cppcoro::sync_wait(task1);
    auto tex2 = cppcoro::sync_wait(task2);

    EXPECT_EQ(tex1, tex2);
    EXPECT_EQ(tex1->GetState(), Obscura::AssetState::Ready);

    std::filesystem::remove(testPath);
}
