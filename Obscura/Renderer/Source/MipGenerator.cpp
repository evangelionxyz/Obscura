#include "MipGenerator.hpp"

#include <cstring>

namespace Obscura
{
    std::vector<MipLevelData> CPUMipGenerator::GenerateMipChain(const void* baseData,
                                                                uint32_t baseWidth,
                                                                uint32_t baseHeight,
                                                                uint32_t baseRowPitch,
                                                                VkFormat format,
                                                                uint32_t mipLevels)
    {
        std::vector<MipLevelData> mipChain;
        if (!baseData || baseWidth == 0 || baseHeight == 0 || mipLevels == 0)
        {
            return mipChain;
        }

        mipChain.reserve(mipLevels);

        // Base mip level (mip 0)
        MipLevelData baseMip;
        baseMip.width = baseWidth;
        baseMip.height = baseHeight;
        baseMip.rowPitch = (baseRowPitch > 0) ? baseRowPitch : (baseWidth * GetBytesPerPixel(format));
        baseMip.slicePitch = baseMip.rowPitch * baseHeight;
        baseMip.data.resize(baseMip.slicePitch);
        std::memcpy(baseMip.data.data(), baseData, baseMip.slicePitch);
        mipChain.push_back(std::move(baseMip));

        // Generate subsequent mip levels
        for (uint32_t mip = 1; mip < mipLevels; ++mip)
        {
            const MipLevelData& srcMip = mipChain[mip - 1];
            MipLevelData dstMip = GenerateNextMipLevel(srcMip, format);
            mipChain.push_back(std::move(dstMip));
        }

        return mipChain;
    }

    MipLevelData CPUMipGenerator::GenerateNextMipLevel(const MipLevelData& srcMip, VkFormat format)
    {
        MipLevelData dstMip;
        dstMip.width = std::max(1u, srcMip.width / 2);
        dstMip.height = std::max(1u, srcMip.height / 2);

        uint32_t bytesPerPixel = GetBytesPerPixel(format);
        dstMip.rowPitch = dstMip.width * bytesPerPixel;
        dstMip.slicePitch = dstMip.rowPitch * dstMip.height;
        dstMip.data.resize(dstMip.slicePitch);

        switch (format)
        {
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
            DownsampleRGBA8(srcMip, dstMip);
            break;
        case VK_FORMAT_R8G8_UNORM:
            DownsampleRG8(srcMip, dstMip);
            break;
        case VK_FORMAT_R8_UNORM:
            DownsampleR8(srcMip, dstMip);
            break;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            DownsampleRGBA16_FLOAT(srcMip, dstMip);
            break;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            DownsampleRGBA32_FLOAT(srcMip, dstMip);
            break;
        default:
            DownsampleRGBA8(srcMip, dstMip);
            break;
        }

        return dstMip;
    }

    uint32_t CPUMipGenerator::GetBytesPerPixel(VkFormat format)
    {
        switch (format)
        {
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
            return 4;
        case VK_FORMAT_R8G8_UNORM:
            return 2;
        case VK_FORMAT_R8_UNORM:
            return 1;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return 8;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 16;
        default:
            return 4;
        }
    }

    void CPUMipGenerator::DownsampleRGBA8(const MipLevelData& src, MipLevelData& dst)
    {
        const uint8_t* srcData = src.data.data();
        uint8_t* dstData = dst.data.data();

        for (uint32_t y = 0; y < dst.height; ++y)
        {
            for (uint32_t x = 0; x < dst.width; ++x)
            {
                uint32_t srcX = x * 2;
                uint32_t srcY = y * 2;

                uint32_t r = 0, g = 0, b = 0, a = 0;
                uint32_t sampleCount = 0;

                for (uint32_t dy = 0; dy < 2 && (srcY + dy) < src.height; ++dy)
                {
                    for (uint32_t dx = 0; dx < 2 && (srcX + dx) < src.width; ++dx)
                    {
                        uint32_t srcOffset = (srcY + dy) * src.rowPitch + (srcX + dx) * 4;
                        r += srcData[srcOffset + 0];
                        g += srcData[srcOffset + 1];
                        b += srcData[srcOffset + 2];
                        a += srcData[srcOffset + 3];
                        sampleCount++;
                    }
                }

                uint32_t dstOffset = y * dst.rowPitch + x * 4;
                dstData[dstOffset + 0] = static_cast<uint8_t>(r / sampleCount);
                dstData[dstOffset + 1] = static_cast<uint8_t>(g / sampleCount);
                dstData[dstOffset + 2] = static_cast<uint8_t>(b / sampleCount);
                dstData[dstOffset + 3] = static_cast<uint8_t>(a / sampleCount);
            }
        }
    }

    void CPUMipGenerator::DownsampleRG8(const MipLevelData& src, MipLevelData& dst)
    {
        const uint8_t* srcData = src.data.data();
        uint8_t* dstData = dst.data.data();

        for (uint32_t y = 0; y < dst.height; ++y)
        {
            for (uint32_t x = 0; x < dst.width; ++x)
            {
                uint32_t srcX = x * 2;
                uint32_t srcY = y * 2;

                uint32_t r = 0, g = 0;
                uint32_t sampleCount = 0;

                for (uint32_t dy = 0; dy < 2 && (srcY + dy) < src.height; ++dy)
                {
                    for (uint32_t dx = 0; dx < 2 && (srcX + dx) < src.width; ++dx)
                    {
                        uint32_t srcOffset = (srcY + dy) * src.rowPitch + (srcX + dx) * 2;
                        r += srcData[srcOffset + 0];
                        g += srcData[srcOffset + 1];
                        sampleCount++;
                    }
                }

                uint32_t dstOffset = y * dst.rowPitch + x * 2;
                dstData[dstOffset + 0] = static_cast<uint8_t>(r / sampleCount);
                dstData[dstOffset + 1] = static_cast<uint8_t>(g / sampleCount);
            }
        }
    }

    void CPUMipGenerator::DownsampleR8(const MipLevelData& src, MipLevelData& dst)
    {
        const uint8_t* srcData = src.data.data();
        uint8_t* dstData = dst.data.data();

        for (uint32_t y = 0; y < dst.height; ++y)
        {
            for (uint32_t x = 0; x < dst.width; ++x)
            {
                uint32_t srcX = x * 2;
                uint32_t srcY = y * 2;

                uint32_t r = 0;
                uint32_t sampleCount = 0;

                for (uint32_t dy = 0; dy < 2 && (srcY + dy) < src.height; ++dy)
                {
                    for (uint32_t dx = 0; dx < 2 && (srcX + dx) < src.width; ++dx)
                    {
                        uint32_t srcOffset = (srcY + dy) * src.rowPitch + (srcX + dx);
                        r += srcData[srcOffset];
                        sampleCount++;
                    }
                }

                uint32_t dstOffset = y * dst.rowPitch + x;
                dstData[dstOffset] = static_cast<uint8_t>(r / sampleCount);
            }
        }
    }

    void CPUMipGenerator::DownsampleRGBA16_FLOAT(const MipLevelData& src, MipLevelData& dst)
    {
        const auto* srcData = reinterpret_cast<const uint16_t*>(src.data.data());
        auto* dstData = reinterpret_cast<uint16_t*>(dst.data.data());
        uint32_t srcRowPitchInPixels = src.rowPitch / 8;
        uint32_t dstRowPitchInPixels = dst.rowPitch / 8;

        for (uint32_t y = 0; y < dst.height; ++y)
        {
            for (uint32_t x = 0; x < dst.width; ++x)
            {
                uint32_t srcX = x * 2;
                uint32_t srcY = y * 2;

                float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
                uint32_t sampleCount = 0;

                for (uint32_t dy = 0; dy < 2 && (srcY + dy) < src.height; ++dy)
                {
                    for (uint32_t dx = 0; dx < 2 && (srcX + dx) < src.width; ++dx)
                    {
                        uint32_t srcIdx = (srcY + dy) * srcRowPitchInPixels + (srcX + dx) * 4;
                        r += HalfToFloat(srcData[srcIdx + 0]);
                        g += HalfToFloat(srcData[srcIdx + 1]);
                        b += HalfToFloat(srcData[srcIdx + 2]);
                        a += HalfToFloat(srcData[srcIdx + 3]);
                        sampleCount++;
                    }
                }

                uint32_t dstIdx = y * dstRowPitchInPixels + x * 4;
                dstData[dstIdx + 0] = FloatToHalf(r / sampleCount);
                dstData[dstIdx + 1] = FloatToHalf(g / sampleCount);
                dstData[dstIdx + 2] = FloatToHalf(b / sampleCount);
                dstData[dstIdx + 3] = FloatToHalf(a / sampleCount);
            }
        }
    }

    void CPUMipGenerator::DownsampleRGBA32_FLOAT(const MipLevelData& src, MipLevelData& dst)
    {
        const auto* srcData = reinterpret_cast<const float*>(src.data.data());
        auto* dstData = reinterpret_cast<float*>(dst.data.data());
        uint32_t srcRowPitchInPixels = src.rowPitch / 16;
        uint32_t dstRowPitchInPixels = dst.rowPitch / 16;

        for (uint32_t y = 0; y < dst.height; ++y)
        {
            for (uint32_t x = 0; x < dst.width; ++x)
            {
                uint32_t srcX = x * 2;
                uint32_t srcY = y * 2;

                float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
                uint32_t sampleCount = 0;

                for (uint32_t dy = 0; dy < 2 && (srcY + dy) < src.height; ++dy)
                {
                    for (uint32_t dx = 0; dx < 2 && (srcX + dx) < src.width; ++dx)
                    {
                        uint32_t srcIdx = (srcY + dy) * srcRowPitchInPixels + (srcX + dx) * 4;
                        r += srcData[srcIdx + 0];
                        g += srcData[srcIdx + 1];
                        b += srcData[srcIdx + 2];
                        a += srcData[srcIdx + 3];
                        sampleCount++;
                    }
                }

                uint32_t dstIdx = y * dstRowPitchInPixels + x * 4;
                dstData[dstIdx + 0] = r / sampleCount;
                dstData[dstIdx + 1] = g / sampleCount;
                dstData[dstIdx + 2] = b / sampleCount;
                dstData[dstIdx + 3] = a / sampleCount;
            }
        }
    }

    float CPUMipGenerator::HalfToFloat(uint16_t h)
    {
        uint32_t sign = (h & 0x8000) << 16;
        uint32_t exponent = (h & 0x7C00) >> 10;
        uint32_t mantissa = h & 0x03FF;

        if (exponent == 0)
        {
            if (mantissa == 0) return 0.0f;
            exponent = 127 - 14;
            while ((mantissa & 0x400) == 0)
            {
                mantissa <<= 1;
                exponent--;
            }
            mantissa &= 0x3FF;
        }
        else if (exponent == 31)
        {
            exponent = 255;
        }
        else
        {
            exponent += 127 - 15;
        }

        uint32_t result = sign | (exponent << 23) | (mantissa << 13);
        float outVal = 0.0f;
        std::memcpy(&outVal, &result, sizeof(float));
        return outVal;
    }

    uint16_t CPUMipGenerator::FloatToHalf(float f)
    {
        uint32_t bits = 0;
        std::memcpy(&bits, &f, sizeof(uint32_t));
        uint32_t sign = (bits & 0x80000000) >> 16;
        uint32_t exponent = (bits & 0x7F800000) >> 23;
        uint32_t mantissa = bits & 0x007FFFFF;

        if (exponent == 0) return static_cast<uint16_t>(sign);
        if (exponent == 255) return static_cast<uint16_t>(sign | 0x7C00 | (mantissa ? 0x200 : 0));

        int32_t exp = static_cast<int32_t>(exponent) - 127 + 15;
        if (exp <= 0) return static_cast<uint16_t>(sign);
        if (exp >= 31) return static_cast<uint16_t>(sign | 0x7C00);

        return static_cast<uint16_t>(sign | (exp << 10) | (mantissa >> 13));
    }
}
