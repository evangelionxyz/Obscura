#pragma once

#include <Obscura/API.hpp>

#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>
    #ifndef VK_USE_PLATFORM_WIN32_KHR
        #define VK_USE_PLATFORM_WIN32_KHR
    #endif
#endif
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace Obscura
{
    struct MipLevelData
    {
        std::vector<uint8_t> data;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t rowPitch = 0;
        uint32_t slicePitch = 0;
    };

    class OBSCURA_RENDERER_API CPUMipGenerator
    {
    public:
        static uint32_t CalculateMaxMipLevels(uint32_t width, uint32_t height, uint32_t depth = 1)
        {
            const uint32_t maxDim = std::max({ width, height, depth, 1u });
            return 1u + static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(maxDim))));
        }

        static std::vector<MipLevelData> GenerateMipChain(const void* baseData,
                                                          uint32_t baseWidth,
                                                          uint32_t baseHeight,
                                                          uint32_t baseRowPitch,
                                                          VkFormat format,
                                                          uint32_t mipLevels);

        static MipLevelData GenerateNextMipLevel(const MipLevelData& srcMip, VkFormat format);
        static uint32_t GetBytesPerPixel(VkFormat format);

    private:
        static void DownsampleRGBA8(const MipLevelData& src, MipLevelData& dst);
        static void DownsampleRG8(const MipLevelData& src, MipLevelData& dst);
        static void DownsampleR8(const MipLevelData& src, MipLevelData& dst);
        static void DownsampleRGBA16_FLOAT(const MipLevelData& src, MipLevelData& dst);
        static void DownsampleRGBA32_FLOAT(const MipLevelData& src, MipLevelData& dst);

        static float HalfToFloat(uint16_t h);
        static uint16_t FloatToHalf(float f);
    };
}
