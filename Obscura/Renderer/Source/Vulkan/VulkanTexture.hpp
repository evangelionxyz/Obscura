#pragma once

#include <Obscura/API.hpp>
#include <Obscura/Types.hpp>

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

#include <cstdint>
#include <filesystem>
#include <string>

namespace Obscura
{
    enum class TextureFilter
    {
        Nearest,
        Linear
    };

    enum class TextureAddressMode
    {
        Repeat,
        ClampToEdge,
        ClampToBorder,
        MirroredRepeat
    };

    struct TextureSamplerDesc
    {
        TextureFilter minFilter = TextureFilter::Linear;
        TextureFilter magFilter = TextureFilter::Linear;
        TextureFilter mipmapMode = TextureFilter::Linear;
        TextureAddressMode addressModeU = TextureAddressMode::Repeat;
        TextureAddressMode addressModeV = TextureAddressMode::Repeat;
        TextureAddressMode addressModeW = TextureAddressMode::Repeat;
        float maxAnisotropy = 16.0f;
        bool enableAnisotropy = true;
    };

    class OBSCURA_RENDERER_API VulkanTexture
    {
    public:
        VulkanTexture() = default;
        ~VulkanTexture() { Destroy(); }

        VulkanTexture(const VulkanTexture&) = delete;
        VulkanTexture& operator=(const VulkanTexture&) = delete;
        VulkanTexture(VulkanTexture&& other) noexcept;
        VulkanTexture& operator=(VulkanTexture&& other) noexcept;

        // Create texture from raw RGBA pixel data
        bool Create(VkDevice device,
                    VkPhysicalDevice physicalDevice,
                    VkCommandPool commandPool,
                    VkQueue queue,
                    uint32_t width,
                    uint32_t height,
                    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
                    const void* pixelData = nullptr,
                    const TextureSamplerDesc& samplerDesc = {});

        // Create texture from image file via stb_image
        bool LoadFromFile(VkDevice device,
                          VkPhysicalDevice physicalDevice,
                          VkCommandPool commandPool,
                          VkQueue queue,
                          const std::filesystem::path& filePath,
                          const TextureSamplerDesc& samplerDesc = {});

        // Create texture from memory buffer via stb_image
        bool LoadFromMemory(VkDevice device,
                            VkPhysicalDevice physicalDevice,
                            VkCommandPool commandPool,
                            VkQueue queue,
                            const void* data,
                            size_t size,
                            const TextureSamplerDesc& samplerDesc = {});

        void Destroy();

        [[nodiscard]] bool IsValid() const noexcept { return m_Image != VK_NULL_HANDLE && m_ImageView != VK_NULL_HANDLE; }
        [[nodiscard]] uint32_t GetWidth() const noexcept { return m_Width; }
        [[nodiscard]] uint32_t GetHeight() const noexcept { return m_Height; }
        [[nodiscard]] VkFormat GetFormat() const noexcept { return m_Format; }
        [[nodiscard]] VkImage GetImage() const noexcept { return m_Image; }
        [[nodiscard]] VkImageView GetImageView() const noexcept { return m_ImageView; }
        [[nodiscard]] VkSampler GetSampler() const noexcept { return m_Sampler; }
        [[nodiscard]] VkImageLayout GetImageLayout() const noexcept { return m_ImageLayout; }
        [[nodiscard]] VkDescriptorImageInfo GetDescriptorInfo() const noexcept
        {
            VkDescriptorImageInfo info{};
            info.sampler = m_Sampler;
            info.imageView = m_ImageView;
            info.imageLayout = m_ImageLayout;
            return info;
        }

    private:
        bool CreateSampler(const TextureSamplerDesc& desc);

        VkDevice         m_Device         = VK_NULL_HANDLE;
        VkImage          m_Image          = VK_NULL_HANDLE;
        VkDeviceMemory   m_Memory         = VK_NULL_HANDLE;
        VkImageView      m_ImageView      = VK_NULL_HANDLE;
        VkSampler        m_Sampler        = VK_NULL_HANDLE;
        VkFormat         m_Format         = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageLayout    m_ImageLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
        uint32_t         m_Width          = 0;
        uint32_t         m_Height         = 0;
        uint32_t         m_MipLevels      = 1;
    };
}
