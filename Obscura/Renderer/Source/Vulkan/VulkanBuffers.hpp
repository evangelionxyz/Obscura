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

#include <cstddef>
#include <cstdint>

namespace Obscura
{
    // Allocates a device-local VkBuffer and uploads data via a host-visible staging buffer.
    // Use VulkanVertexBuffer for vertex data and VulkanIndexBuffer for index data.

    class OBSCURA_RENDERER_API VulkanVertexBuffer
    {
    public:
        VulkanVertexBuffer() = default;
        ~VulkanVertexBuffer() = default;

        // Upload 'data' (size bytes) to a device-local vertex buffer.
        // commandPool and queue are used for the one-shot staging copy.
        bool Create(VkDevice device,
                    VkPhysicalDevice physicalDevice,
                    VkCommandPool commandPool,
                    VkQueue queue,
                    const void* data,
                    std::size_t size);

        // Allocate a persistently mapped host-visible dynamic vertex buffer.
        bool CreateDynamic(VkDevice device,
                           VkPhysicalDevice physicalDevice,
                           std::size_t size);

        // Update data in dynamic vertex buffer via fast memory copy.
        void SetData(const void* data, std::size_t size, std::size_t offset = 0);

        void Destroy();

        [[nodiscard]] bool       IsValid()       const noexcept { return m_Buffer != VK_NULL_HANDLE; }
        [[nodiscard]] VkBuffer   GetBuffer()     const noexcept { return m_Buffer; }
        [[nodiscard]] std::size_t GetSize()      const noexcept { return m_Size; }
        [[nodiscard]] void*      GetMappedData() const noexcept { return m_Mapped; }

    private:
        VkDevice        m_Device  = VK_NULL_HANDLE;
        VkBuffer        m_Buffer  = VK_NULL_HANDLE;
        VkDeviceMemory  m_Memory  = VK_NULL_HANDLE;
        void*           m_Mapped  = nullptr;
        std::size_t     m_Size    = 0;
    };

    class OBSCURA_RENDERER_API VulkanIndexBuffer
    {
    public:
        VulkanIndexBuffer() = default;
        ~VulkanIndexBuffer() = default;

        // Upload 'data' (size bytes) to a device-local index buffer.
        // indexCount is stored for draw calls. indexType defaults to UINT32.
        bool Create(VkDevice device,
                    VkPhysicalDevice physicalDevice,
                    VkCommandPool commandPool,
                    VkQueue queue,
                    const void* data,
                    std::size_t size,
                    uint32_t indexCount,
                    VkIndexType indexType = VK_INDEX_TYPE_UINT32);

        // Allocate a persistently mapped host-visible dynamic index buffer.
        bool CreateDynamic(VkDevice device,
                           VkPhysicalDevice physicalDevice,
                           const void* data,
                           std::size_t size,
                           uint32_t indexCount,
                           VkIndexType indexType = VK_INDEX_TYPE_UINT32);

        // Update data in dynamic index buffer via fast memory copy.
        void SetData(const void* data, std::size_t size, std::size_t offset = 0);

        void Destroy();

        [[nodiscard]] bool        IsValid()       const noexcept { return m_Buffer != VK_NULL_HANDLE; }
        [[nodiscard]] VkBuffer    GetBuffer()     const noexcept { return m_Buffer; }
        [[nodiscard]] uint32_t    GetIndexCount() const noexcept { return m_IndexCount; }
        [[nodiscard]] VkIndexType GetIndexType()  const noexcept { return m_IndexType; }
        [[nodiscard]] void*       GetMappedData() const noexcept { return m_Mapped; }

    private:
        VkDevice        m_Device     = VK_NULL_HANDLE;
        VkBuffer        m_Buffer     = VK_NULL_HANDLE;
        VkDeviceMemory  m_Memory     = VK_NULL_HANDLE;
        void*           m_Mapped     = nullptr;
        uint32_t        m_IndexCount = 0;
        VkIndexType     m_IndexType  = VK_INDEX_TYPE_UINT32;
    };


    class OBSCURA_RENDERER_API VulkanUniformBuffer
    {
    public:
        VulkanUniformBuffer() = default;
        ~VulkanUniformBuffer() = default;

        // Allocate a persistently mapped host-visible dynamic uniform buffer.
        bool Create(VkDevice device,
                           VkPhysicalDevice physicalDevice,
                           std::size_t size);

        // Update data in dynamic index buffer via fast memory copy.
        void SetData(const void *data, std::size_t size, std::size_t offset = 0);

        void Destroy();

        [[nodiscard]] bool        IsValid()       const noexcept { return m_Buffer != VK_NULL_HANDLE; }
        [[nodiscard]] VkBuffer    GetBuffer()     const noexcept { return m_Buffer; }
        [[nodiscard]] uint32_t    GetIndexCount() const noexcept { return m_IndexCount; }
        [[nodiscard]] void *GetMappedData()       const noexcept { return m_Mapped; }

    private:
        VkDevice        m_Device = VK_NULL_HANDLE;
        VkBuffer        m_Buffer = VK_NULL_HANDLE;
        VkDeviceMemory  m_Memory = VK_NULL_HANDLE;
        void *m_Mapped = nullptr;
        uint32_t        m_IndexCount = 0;
        std::size_t     m_Size = 0;
    };
}
