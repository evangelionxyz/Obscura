#pragma once

#include <Obscura/API.hpp>
#include <Obscura/IRHI.hpp>
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

#include <array>
#include <cstdint>
#include <vector>

namespace Obscura
{
    static constexpr std::uint32_t BACKBUFFER_COUNT = 3;

    class OBSCURA_RENDERER_API VulkanRHI : public IRHI
    {
    public:
        VulkanRHI() = default;
        ~VulkanRHI() override = default;

        bool        Initialize() override;
        void        Shutdown() override;
        void        BeginFrame() override;
        void        EndFrame() override;
        void        Destroy() override;
        const char *GetName() const override;

        bool        CreateOffscreenTarget(std::uint32_t width, std::uint32_t height) override;
        void        DestroyOffscreenTarget() override;
        bool        ResizeOffscreenTarget(std::uint32_t width, std::uint32_t height) override;
        const void *GetFrameBufferData() const override;
        std::uint32_t GetFrameBufferStride() const override;
        std::uint32_t GetFrameBufferWidth() const override;
        std::uint32_t GetFrameBufferHeight() const override;
        void        SetRenderMode(RenderMode mode) override;

        GPUTextureHandle GetGPUTextureHandle() const override;
        VulkanDeviceObjects GetVulkanDeviceObjects() const override;
        FrameContext GetCurrentFrameContext() const override;
        const void* GetOffscreenImageView(std::uint32_t index) const override;
        std::uint32_t GetBackbufferCount() const override { return BACKBUFFER_COUNT; }
        int GetOffscreenFormat() const override { return static_cast<int>(m_OffscreenFormat); }

        [[nodiscard]] VkInstance       GetInstance() const noexcept { return m_Instance; }
        [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const noexcept { return m_PhysicalDevice; }
        [[nodiscard]] VkDevice         GetDevice() const noexcept { return m_Device; }
        [[nodiscard]] VkQueue          GetGraphicsQueue() const noexcept { return m_GraphicsQueue; }
        [[nodiscard]] std::uint32_t    GetGraphicsQueueFamily() const noexcept { return m_GraphicsQueueFamily; }
        [[nodiscard]] VkCommandPool    GetCommandPool() const noexcept { return m_CommandPool; }
        [[nodiscard]] bool             IsDescriptorIndexingSupported() const noexcept { return m_DescriptorIndexingSupported; }

    private:
        bool CreateInstance();
        bool SelectPhysicalDevice();
        bool CreateLogicalDevice();
        bool CreateCommandObjects();
        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        bool CheckValidationLayerSupport(const std::vector<const char*>& layers);
        void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        void SetupDebugMessenger();

    private:
        VkInstance               m_Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
        VkPhysicalDevice         m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice                 m_Device = VK_NULL_HANDLE;
        VkQueue                  m_GraphicsQueue = VK_NULL_HANDLE;
        uint32_t                 m_GraphicsQueueFamily = 0;
        uint32_t                 m_AvailableQueueCount = 1;
        uint32_t                 m_QtQueueIndex = 0;

        bool                     m_EnableValidationLayers = true;
        const std::vector<const char*> m_ValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };

        VkCommandPool    m_CommandPool = VK_NULL_HANDLE;
        std::array<VkCommandBuffer, BACKBUFFER_COUNT> m_CommandBuffers{};
        std::array<VkFence, BACKBUFFER_COUNT>         m_RenderFences{};

        // Triple-buffered GPU Offscreen Render Targets
        std::array<VkImage, BACKBUFFER_COUNT>         m_OffscreenImages{};
        std::array<VkDeviceMemory, BACKBUFFER_COUNT>  m_OffscreenMemories{};
        std::array<VkImageView, BACKBUFFER_COUNT>     m_OffscreenImageViews{};
        std::array<VkImageLayout, BACKBUFFER_COUNT>   m_CurrentLayouts{};
        VkFormat         m_OffscreenFormat = VK_FORMAT_R8G8B8A8_UNORM;

        std::uint32_t    m_WriteIndex = 0;
        std::uint32_t    m_ReadIndex = 0;

        RenderMode       m_RenderMode = RenderMode::Window;
        std::uint32_t    m_OffscreenWidth = 0;
        std::uint32_t    m_OffscreenHeight = 0;
        std::vector<std::uint8_t> m_FrameBuffer;
        std::uint32_t    m_FrameCounter = 0;

        bool             m_DescriptorIndexingSupported = false;
        bool             m_Initialized = false;
        bool             m_FrameActive = false;
    };
}
