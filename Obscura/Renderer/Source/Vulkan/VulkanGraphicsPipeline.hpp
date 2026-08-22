#pragma once

#include <Obscura/API.hpp>
#include "VulkanShader.hpp"

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

#include <array>
#include <cstdint>
#include <vector>

namespace Obscura
{
    static constexpr uint32_t PIPELINE_BACKBUFFER_COUNT = 3;

    // Owns a VkPipeline, VkPipelineLayout, VkDescriptorSetLayout(s), VkRenderPass,
    // and per-frame VkFramebuffers. Layout is derived entirely from SPIRV reflection.
    class OBSCURA_RENDERER_API VulkanGraphicsPipeline
    {
    public:
        VulkanGraphicsPipeline() = default;
        ~VulkanGraphicsPipeline() = default;

        // Build the pipeline from reflected shader pair.
        // imageViews: triple-buffered offscreen image views to attach to framebuffers.
        bool Create(VkDevice                                            device,
                    const VulkanShader&                                 vertShader,
                    const VulkanShader&                                 fragShader,
                    VkFormat                                            targetFormat,
                    uint32_t                                            width,
                    uint32_t                                            height,
                    const std::array<VkImageView, PIPELINE_BACKBUFFER_COUNT>& imageViews,
                    const std::vector<VkDescriptorSetLayout>&           externalDescriptorSetLayouts = {});

        // Recreate framebuffers when the offscreen target is resized.
        bool RecreateFramebuffers(const std::array<VkImageView, PIPELINE_BACKBUFFER_COUNT>& imageViews,
                                  uint32_t width, uint32_t height);

        void Destroy();

        [[nodiscard]] bool              IsValid()          const noexcept { return m_Pipeline != VK_NULL_HANDLE; }
        [[nodiscard]] VkPipeline        GetPipeline()      const noexcept { return m_Pipeline; }
        [[nodiscard]] VkPipelineLayout  GetLayout()        const noexcept { return m_PipelineLayout; }
        [[nodiscard]] VkRenderPass      GetRenderPass()    const noexcept { return m_RenderPass; }

        // Returns the framebuffer for the given backbuffer index.
        [[nodiscard]] VkFramebuffer GetFramebuffer(uint32_t index) const noexcept
        {
            return (index < PIPELINE_BACKBUFFER_COUNT) ? m_Framebuffers[index] : VK_NULL_HANDLE;
        }

        [[nodiscard]] uint32_t GetWidth()  const noexcept { return m_Width; }
        [[nodiscard]] uint32_t GetHeight() const noexcept { return m_Height; }

    private:
        bool BuildRenderPass(VkFormat format);
        bool BuildPipelineLayout(const VulkanShader& vertShader, const VulkanShader& fragShader, const std::vector<VkDescriptorSetLayout>& externalLayouts = {});
        bool BuildPipeline(const VulkanShader& vertShader, const VulkanShader& fragShader);
        bool BuildFramebuffers(const std::array<VkImageView, PIPELINE_BACKBUFFER_COUNT>& imageViews,
                               uint32_t width, uint32_t height);
        void DestroyFramebuffers();

        VkDevice         m_Device         = VK_NULL_HANDLE;
        VkPipeline       m_Pipeline       = VK_NULL_HANDLE;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
        VkRenderPass     m_RenderPass     = VK_NULL_HANDLE;

        // One layout per descriptor set (from reflection)
        std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
        bool m_OwnsDescriptorSetLayouts = true;

        std::array<VkFramebuffer, PIPELINE_BACKBUFFER_COUNT> m_Framebuffers{};
        uint32_t m_Width  = 0;
        uint32_t m_Height = 0;
    };
}
