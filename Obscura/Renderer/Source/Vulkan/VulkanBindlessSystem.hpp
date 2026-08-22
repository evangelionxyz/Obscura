#pragma once

#include <Obscura/API.hpp>
#include "VulkanTexture.hpp"

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
#include <mutex>
#include <vector>

namespace Obscura
{
    static constexpr uint32_t MAX_BINDLESS_TEXTURES = 4096;
    static constexpr uint32_t INVALID_BINDLESS_INDEX = 0xFFFFFFFF;

    // Manages global bindless descriptor sets for sampled 2D textures.
    class OBSCURA_RENDERER_API VulkanBindlessSystem
    {
    public:
        static bool Initialize(VkDevice device, uint32_t maxTextures = MAX_BINDLESS_TEXTURES);
        static void Shutdown();

        // Registers a texture into the global bindless descriptor array and returns its slot ID.
        static uint32_t RegisterTexture(const VulkanTexture& texture);
        static uint32_t RegisterTexture(VkImageView imageView, VkSampler sampler);

        // Frees the texture slot for subsequent reuse.
        static void UnregisterTexture(uint32_t slot);

        [[nodiscard]] static VkDescriptorSetLayout GetDescriptorSetLayout() noexcept { return s_DescriptorSetLayout; }
        [[nodiscard]] static VkDescriptorSet       GetDescriptorSet()       noexcept { return s_DescriptorSet; }
        [[nodiscard]] static bool                  IsInitialized()          noexcept { return s_Initialized; }
        [[nodiscard]] static uint32_t              GetMaxTextures()         noexcept { return s_MaxTextures; }

    private:
        static VkDevice              s_Device;
        static VkDescriptorPool      s_DescriptorPool;
        static VkDescriptorSetLayout s_DescriptorSetLayout;
        static VkDescriptorSet       s_DescriptorSet;
        static std::vector<uint32_t> s_FreeSlots;
        static uint32_t              s_NextSlot;
        static uint32_t              s_MaxTextures;
        static std::mutex            s_Mutex;
        static bool                  s_Initialized;
    };
}
