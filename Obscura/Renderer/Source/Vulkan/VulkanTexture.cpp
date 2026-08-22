#include "VulkanTexture.hpp"
#include "MipGenerator.hpp"

#include <Obscura/Logger.hpp>
#include <Obscura/VFS.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstring>
#include <utility>

namespace Obscura
{
    static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1u << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if (typeFilter & (1u << i))
                return i;
        }
        return 0;
    }

    static VkFilter ConvertFilter(TextureFilter filter)
    {
        switch (filter)
        {
            case TextureFilter::Nearest: return VK_FILTER_NEAREST;
            case TextureFilter::Linear:
            default:                     return VK_FILTER_LINEAR;
        }
    }

    static VkSamplerMipmapMode ConvertMipmapMode(TextureFilter filter)
    {
        switch (filter)
        {
            case TextureFilter::Nearest: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
            case TextureFilter::Linear:
            default:                     return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }
    }

    static VkSamplerAddressMode ConvertAddressMode(TextureAddressMode mode)
    {
        switch (mode)
        {
            case TextureAddressMode::ClampToEdge:    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case TextureAddressMode::ClampToBorder:  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            case TextureAddressMode::MirroredRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            case TextureAddressMode::Repeat:
            default:                                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    }

    VulkanTexture::VulkanTexture(VulkanTexture&& other) noexcept
    {
        *this = std::move(other);
    }

    VulkanTexture& VulkanTexture::operator=(VulkanTexture&& other) noexcept
    {
        if (this != &other)
        {
            Destroy();

            m_Device         = other.m_Device;
            m_Image          = other.m_Image;
            m_Memory         = other.m_Memory;
            m_ImageView      = other.m_ImageView;
            m_Sampler        = other.m_Sampler;
            m_Format         = other.m_Format;
            m_ImageLayout    = other.m_ImageLayout;
            m_Width          = other.m_Width;
            m_Height         = other.m_Height;
            m_MipLevels      = other.m_MipLevels;

            other.m_Device      = VK_NULL_HANDLE;
            other.m_Image       = VK_NULL_HANDLE;
            other.m_Memory      = VK_NULL_HANDLE;
            other.m_ImageView   = VK_NULL_HANDLE;
            other.m_Sampler     = VK_NULL_HANDLE;
            other.m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            other.m_Width       = 0;
            other.m_Height      = 0;
            other.m_MipLevels   = 1;
        }
        return *this;
    }

    bool VulkanTexture::Create(VkDevice device,
                               VkPhysicalDevice physicalDevice,
                               VkCommandPool commandPool,
                               VkQueue queue,
                               uint32_t width,
                               uint32_t height,
                               VkFormat format,
                               const void* pixelData,
                               const TextureSamplerDesc& samplerDesc)
    {
        Destroy();

        if (device == VK_NULL_HANDLE || width == 0 || height == 0)
        {
            LOG_ERROR("[VulkanTexture] Invalid parameters for texture creation.");
            return false;
        }

        m_Device = device;
        m_Width  = width;
        m_Height = height;
        m_Format = format;

        // Generate MipChain if requested
        std::vector<MipLevelData> mipChain;
        if (pixelData && samplerDesc.generateMipmaps)
        {
            uint32_t maxMips = CPUMipGenerator::CalculateMaxMipLevels(width, height);
            uint32_t bpp = CPUMipGenerator::GetBytesPerPixel(format);
            uint32_t baseRowPitch = width * bpp;
            mipChain = CPUMipGenerator::GenerateMipChain(pixelData, width, height, baseRowPitch, format, maxMips);
            m_MipLevels = static_cast<uint32_t>(mipChain.size());
        }
        else
        {
            m_MipLevels = 1;
        }

        if (m_MipLevels == 0)
        {
            m_MipLevels = 1;
        }

        // 1. Create VkImage
        VkImageCreateInfo imageInfo{};
        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = m_MipLevels;
        imageInfo.arrayLayers   = 1;
        imageInfo.format        = format;
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

        if (vkCreateImage(m_Device, &imageInfo, nullptr, &m_Image) != VK_SUCCESS)
        {
            LOG_ERROR("[VulkanTexture] Failed to create VkImage.");
            return false;
        }

        // 2. Allocate and bind device memory
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(m_Device, m_Image, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memReqs.size;
        allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_Memory) != VK_SUCCESS)
        {
            LOG_ERROR("[VulkanTexture] Failed to allocate device memory for image.");
            Destroy();
            return false;
        }

        vkBindImageMemory(m_Device, m_Image, m_Memory, 0);

        // 3. Staging buffer copy and transition if pixelData provided
        if (pixelData && commandPool != VK_NULL_HANDLE && queue != VK_NULL_HANDLE)
        {
            VkDeviceSize totalStagingSize = 0;
            std::vector<VkDeviceSize> mipOffsets;

            if (!mipChain.empty())
            {
                for (const auto& mip : mipChain)
                {
                    mipOffsets.push_back(totalStagingSize);
                    totalStagingSize += mip.data.size();
                }
            }
            else
            {
                totalStagingSize = static_cast<VkDeviceSize>(width) * height * CPUMipGenerator::GetBytesPerPixel(format);
                mipOffsets.push_back(0);
            }

            VkBuffer stagingBuffer = VK_NULL_HANDLE;
            VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

            VkBufferCreateInfo bufInfo{};
            bufInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufInfo.size        = totalStagingSize;
            bufInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(m_Device, &bufInfo, nullptr, &stagingBuffer) != VK_SUCCESS)
            {
                LOG_ERROR("[VulkanTexture] Failed to create staging buffer.");
                Destroy();
                return false;
            }

            VkMemoryRequirements stageReqs;
            vkGetBufferMemoryRequirements(m_Device, stagingBuffer, &stageReqs);

            VkMemoryAllocateInfo stageAlloc{};
            stageAlloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            stageAlloc.allocationSize  = stageReqs.size;
            stageAlloc.memoryTypeIndex = FindMemoryType(physicalDevice, stageReqs.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            if (vkAllocateMemory(m_Device, &stageAlloc, nullptr, &stagingMemory) != VK_SUCCESS)
            {
                LOG_ERROR("[VulkanTexture] Failed to allocate staging memory.");
                vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
                Destroy();
                return false;
            }

            vkBindBufferMemory(m_Device, stagingBuffer, stagingMemory, 0);

            void* mapped = nullptr;
            vkMapMemory(m_Device, stagingMemory, 0, totalStagingSize, 0, &mapped);
            if (!mipChain.empty())
            {
                for (size_t i = 0; i < mipChain.size(); ++i)
                {
                    std::memcpy(static_cast<uint8_t*>(mapped) + mipOffsets[i],
                                mipChain[i].data.data(),
                                mipChain[i].data.size());
                }
            }
            else
            {
                std::memcpy(mapped, pixelData, static_cast<size_t>(totalStagingSize));
            }
            vkUnmapMemory(m_Device, stagingMemory);

            // Execute single-time command buffer for layout transitions and copy
            VkCommandBufferAllocateInfo cmdAllocInfo{};
            cmdAllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdAllocInfo.commandPool        = commandPool;
            cmdAllocInfo.commandBufferCount = 1;

            VkCommandBuffer cmd = VK_NULL_HANDLE;
            vkAllocateCommandBuffers(m_Device, &cmdAllocInfo, &cmd);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(cmd, &beginInfo);

            // Transition: UNDEFINED -> TRANSFER_DST_OPTIMAL (all mips)
            VkImageMemoryBarrier barrier{};
            barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
            barrier.image                           = m_Image;
            barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel   = 0;
            barrier.subresourceRange.levelCount     = m_MipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount     = 1;
            barrier.srcAccessMask                   = 0;
            barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(cmd,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            // Copy buffer to image for all mip levels
            std::vector<VkBufferImageCopy> regions;
            if (!mipChain.empty())
            {
                for (uint32_t i = 0; i < m_MipLevels; ++i)
                {
                    VkBufferImageCopy region{};
                    region.bufferOffset                    = mipOffsets[i];
                    region.bufferRowLength                 = 0;
                    region.bufferImageHeight               = 0;
                    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                    region.imageSubresource.mipLevel       = i;
                    region.imageSubresource.baseArrayLayer = 0;
                    region.imageSubresource.layerCount     = 1;
                    region.imageOffset                     = { 0, 0, 0 };
                    region.imageExtent                     = { mipChain[i].width, mipChain[i].height, 1 };
                    regions.push_back(region);
                }
            }
            else
            {
                VkBufferImageCopy region{};
                region.bufferOffset                    = 0;
                region.bufferRowLength                 = 0;
                region.bufferImageHeight               = 0;
                region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel       = 0;
                region.imageSubresource.baseArrayLayer = 0;
                region.imageSubresource.layerCount     = 1;
                region.imageOffset                     = { 0, 0, 0 };
                region.imageExtent                     = { width, height, 1 };
                regions.push_back(region);
            }

            vkCmdCopyBufferToImage(cmd, stagingBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   static_cast<uint32_t>(regions.size()), regions.data());

            // Transition: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL (all mips)
            barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmd,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            vkEndCommandBuffer(cmd);

            VkSubmitInfo submitInfo{};
            submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers    = &cmd;

            vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(queue);

            vkFreeCommandBuffers(m_Device, commandPool, 1, &cmd);
            vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
            vkFreeMemory(m_Device, stagingMemory, nullptr);

            m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
        else
        {
            m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        // 4. Create VkImageView
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = m_Image;
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = format;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = m_MipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(m_Device, &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS)
        {
            LOG_ERROR("[VulkanTexture] Failed to create VkImageView.");
            Destroy();
            return false;
        }

        // 5. Create VkSampler
        if (!CreateSampler(samplerDesc))
        {
            LOG_ERROR("[VulkanTexture] Failed to create VkSampler.");
            Destroy();
            return false;
        }

        return true;
    }

    bool VulkanTexture::CreateSampler(const TextureSamplerDesc& desc)
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter               = ConvertFilter(desc.magFilter);
        samplerInfo.minFilter               = ConvertFilter(desc.minFilter);
        samplerInfo.mipmapMode              = ConvertMipmapMode(desc.mipmapMode);
        samplerInfo.addressModeU            = ConvertAddressMode(desc.addressModeU);
        samplerInfo.addressModeV            = ConvertAddressMode(desc.addressModeV);
        samplerInfo.addressModeW            = ConvertAddressMode(desc.addressModeW);
        samplerInfo.mipLodBias              = 0.0f;
        samplerInfo.anisotropyEnable        = desc.enableAnisotropy ? VK_TRUE : VK_FALSE;
        samplerInfo.maxAnisotropy           = desc.maxAnisotropy;
        samplerInfo.compareEnable           = VK_FALSE;
        samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
        samplerInfo.minLod                  = 0.0f;
        samplerInfo.maxLod                  = static_cast<float>(m_MipLevels > 1 ? (m_MipLevels - 1) : 0);
        samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        return vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_Sampler) == VK_SUCCESS;
    }

    bool VulkanTexture::LoadFromFile(VkDevice device,
                                     VkPhysicalDevice physicalDevice,
                                     VkCommandPool commandPool,
                                     VkQueue queue,
                                     const std::filesystem::path& filePath,
                                     const TextureSamplerDesc& samplerDesc)
    {
        std::filesystem::path resolved = VFS::ResolvePath(filePath.string());
        if (resolved.empty())
        {
            resolved = filePath;
        }

        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* pixels = stbi_load(resolved.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!pixels)
        {
            LOG_ERROR("[VulkanTexture] Failed to load image file '{}': {}", resolved.string(), stbi_failure_reason());
            return false;
        }

        bool result = Create(device, physicalDevice, commandPool, queue,
                             static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                             VK_FORMAT_R8G8B8A8_UNORM, pixels, samplerDesc);

        stbi_image_free(pixels);
        return result;
    }

    bool VulkanTexture::LoadFromMemory(VkDevice device,
                                       VkPhysicalDevice physicalDevice,
                                       VkCommandPool commandPool,
                                       VkQueue queue,
                                       const void* data,
                                       size_t size,
                                       const TextureSamplerDesc& samplerDesc)
    {
        if (!data || size == 0)
        {
            LOG_ERROR("[VulkanTexture] Invalid memory buffer for texture decoding.");
            return false;
        }

        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* pixels = stbi_load_from_memory(static_cast<const stbi_uc*>(data), static_cast<int>(size),
                                                &width, &height, &channels, STBI_rgb_alpha);

        if (!pixels)
        {
            LOG_ERROR("[VulkanTexture] Failed to decode image from memory: {}", stbi_failure_reason());
            return false;
        }

        bool result = Create(device, physicalDevice, commandPool, queue,
                             static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                             VK_FORMAT_R8G8B8A8_UNORM, pixels, samplerDesc);

        stbi_image_free(pixels);
        return result;
    }

    void VulkanTexture::Destroy()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            if (m_Sampler != VK_NULL_HANDLE)
            {
                vkDestroySampler(m_Device, m_Sampler, nullptr);
                m_Sampler = VK_NULL_HANDLE;
            }
            if (m_ImageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(m_Device, m_ImageView, nullptr);
                m_ImageView = VK_NULL_HANDLE;
            }
            if (m_Image != VK_NULL_HANDLE)
            {
                vkDestroyImage(m_Device, m_Image, nullptr);
                m_Image = VK_NULL_HANDLE;
            }
            if (m_Memory != VK_NULL_HANDLE)
            {
                vkFreeMemory(m_Device, m_Memory, nullptr);
                m_Memory = VK_NULL_HANDLE;
            }
        }

        m_Device      = VK_NULL_HANDLE;
        m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_Width       = 0;
        m_Height      = 0;
        m_MipLevels   = 1;
    }
}
