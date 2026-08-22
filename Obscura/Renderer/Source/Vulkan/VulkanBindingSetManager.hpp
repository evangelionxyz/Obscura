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

#include <cstdint>
#include <vector>

namespace Obscura
{
    struct DescriptorBindingItem
    {
        uint32_t           binding         = 0;
        VkDescriptorType   type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        VkBuffer           buffer          = VK_NULL_HANDLE;
        VkDeviceSize       offset          = 0;
        VkDeviceSize       range           = 0;
        VkImageView        imageView       = VK_NULL_HANDLE;
        VkSampler          sampler         = VK_NULL_HANDLE;
        VkImageLayout      imageLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        bool operator==(const DescriptorBindingItem& other) const noexcept
        {
            return binding == other.binding &&
                   type == other.type &&
                   buffer == other.buffer &&
                   offset == other.offset &&
                   range == other.range &&
                   imageView == other.imageView &&
                   sampler == other.sampler &&
                   imageLayout == other.imageLayout;
        }
    };

    struct DescriptorBindingSetDesc
    {
        std::vector<DescriptorBindingItem> bindings;

        bool operator==(const DescriptorBindingSetDesc& other) const noexcept
        {
            return bindings == other.bindings;
        }
    };

    // Thread-safe descriptor set caching manager to avoid redundant Vulkan descriptor allocations.
    class OBSCURA_RENDERER_API VulkanBindingSetManager
    {
    public:
        static void Initialize(VkDevice device, VkDescriptorPool descriptorPool);
        static void Shutdown();
        static void Clear();

        static VkDescriptorSet GetCachedBindingSet(const DescriptorBindingSetDesc& desc, VkDescriptorSetLayout layout);
        static VkDescriptorSet GetOrCreateBindingSet(const DescriptorBindingSetDesc& desc, VkDescriptorSetLayout layout);
        static void            RemoveBindingSet(const DescriptorBindingSetDesc& desc, VkDescriptorSetLayout layout);

        [[nodiscard]] static bool IsInitialized() noexcept;
    };
}
