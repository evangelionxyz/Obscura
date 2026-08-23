#include "VulkanBuffers.hpp"
#include "VulkanUtils.hpp"

#include <Obscura/Logger.hpp>

namespace Obscura
{
    // ---------------------------------------------------------------------------
    // VulkanVertexBuffer
    // ---------------------------------------------------------------------------
    bool VulkanVertexBuffer::Create(VkDevice         device,
                                    VkPhysicalDevice physicalDevice,
                                    VkCommandPool    commandPool,
                                    VkQueue          queue,
                                    const void*      data,
                                    std::size_t      size)
    {
        m_Device = device;
        m_Size   = size;

        bool ok = VulkanUtils::UploadToDeviceLocal(device, physicalDevice,
                                      commandPool, queue,
                                      data, size,
                                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                      m_Buffer, m_Memory);
        if (!ok)
        {
            LOG_ASSERT(false, "[VulkanVertexBuffer] Failed to create device-local vertex buffer ({} bytes)", size);
        }
        else
        {
            LOG_INFO("[VulkanVertexBuffer] Created vertex buffer ({} bytes)", size);
        }
        return ok;
    }

    bool VulkanVertexBuffer::CreateDynamic(VkDevice         device,
                                           VkPhysicalDevice physicalDevice,
                                           std::size_t      size)
    {
        Destroy();

        m_Device = device;
        m_Size   = size;

        if (!VulkanUtils::CreateRawBuffer(device, physicalDevice,
                             static_cast<VkDeviceSize>(size),
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             m_Buffer, m_Memory))
        {
            LOG_ASSERT(false, "[VulkanVertexBuffer] Failed to create dynamic vertex buffer ({} bytes)", size);
            return false;
        }

        if (vkMapMemory(device, m_Memory, 0, static_cast<VkDeviceSize>(size), 0, &m_Mapped) != VK_SUCCESS)
        {
            LOG_ASSERT(false, "[VulkanVertexBuffer] Failed to map dynamic vertex buffer memory.");
            Destroy();
            return false;
        }

        LOG_INFO("[VulkanVertexBuffer] Created dynamic vertex buffer ({} bytes, persistently mapped)", size);
        return true;
    }

    void VulkanVertexBuffer::SetData(const void* data, std::size_t size, std::size_t offset)
    {
        if (m_Mapped && data && size > 0 && offset + size <= m_Size)
        {
            std::memcpy(static_cast<uint8_t*>(m_Mapped) + offset, data, size);
        }
        else
        {
            LOG_ERROR("[VulkanVertexBuffer] SetData out of bounds or buffer unmapped (size: {}, offset: {}, bufferSize: {})",
                      size, offset, m_Size);
        }
    }

    void VulkanVertexBuffer::Destroy()
    {
        if (m_Device == VK_NULL_HANDLE)
            return;

        if (m_Mapped != nullptr && m_Memory != VK_NULL_HANDLE)
        {
            vkUnmapMemory(m_Device, m_Memory);
            m_Mapped = nullptr;
        }

        if (m_Buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(m_Device, m_Buffer, nullptr);
            m_Buffer = VK_NULL_HANDLE;
        }
        if (m_Memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_Device, m_Memory, nullptr);
            m_Memory = VK_NULL_HANDLE;
        }
        m_Size = 0;
    }

    // ---------------------------------------------------------------------------
    // VulkanIndexBuffer
    // ---------------------------------------------------------------------------
    bool VulkanIndexBuffer::Create(VkDevice         device,
                                   VkPhysicalDevice physicalDevice,
                                   VkCommandPool    commandPool,
                                   VkQueue          queue,
                                   const void*      data,
                                   std::size_t      size,
                                   uint32_t         indexCount,
                                   VkIndexType      indexType)
    {
        m_Device     = device;
        m_IndexCount = indexCount;
        m_IndexType  = indexType;

        bool ok = VulkanUtils::UploadToDeviceLocal(device, physicalDevice, commandPool, queue,
                                      data, size,
                                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                      m_Buffer, m_Memory);
        if (!ok)
        {
            LOG_ERROR("[VulkanIndexBuffer] Failed to create device-local index buffer ({} bytes)", size);
        }
        else
        {
            LOG_INFO("[VulkanIndexBuffer] Created index buffer ({} indices)", indexCount);
        }
        return ok;
    }

    bool VulkanIndexBuffer::CreateDynamic(VkDevice         device,
                                          VkPhysicalDevice physicalDevice,
                                          const void*      data,
                                          std::size_t      size,
                                          uint32_t         indexCount,
                                          VkIndexType      indexType)
    {
        Destroy();

        m_Device     = device;
        m_IndexCount = indexCount;
        m_IndexType  = indexType;

        if (!VulkanUtils::CreateRawBuffer(device, physicalDevice,
                             static_cast<VkDeviceSize>(size),
                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             m_Buffer, m_Memory))
        {
            LOG_ERROR("[VulkanIndexBuffer] Failed to create dynamic index buffer ({} bytes)", size);
            return false;
        }

        if (vkMapMemory(device, m_Memory, 0, static_cast<VkDeviceSize>(size), 0, &m_Mapped) != VK_SUCCESS)
        {
            LOG_ERROR("[VulkanIndexBuffer] Failed to map dynamic index buffer memory.");
            Destroy();
            return false;
        }

        if (data && size > 0)
        {
            std::memcpy(m_Mapped, data, size);
        }

        LOG_INFO("[VulkanIndexBuffer] Created dynamic index buffer ({} indices, persistently mapped)", indexCount);
        return true;
    }

    void VulkanIndexBuffer::SetData(const void* data, std::size_t size, std::size_t offset)
    {
        if (m_Mapped && data && size > 0)
        {
            std::memcpy(static_cast<uint8_t*>(m_Mapped) + offset, data, size);
        }
    }

    void VulkanIndexBuffer::Destroy()
    {
        if (m_Device == VK_NULL_HANDLE)
            return;

        if (m_Mapped != nullptr && m_Memory != VK_NULL_HANDLE)
        {
            vkUnmapMemory(m_Device, m_Memory);
            m_Mapped = nullptr;
        }

        if (m_Buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(m_Device, m_Buffer, nullptr);
            m_Buffer = VK_NULL_HANDLE;
        }
        if (m_Memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_Device, m_Memory, nullptr);
            m_Memory = VK_NULL_HANDLE;
        }
        m_IndexCount = 0;
    }


    // ---------------------------------------------------------------------------
    // VulkanUniformBuffer
    // ---------------------------------------------------------------------------
    bool VulkanUniformBuffer::Create(VkDevice        device,
                                    VkPhysicalDevice physicalDevice,
                                    std::size_t      size)
    {
        Destroy();

        m_Device = device;
        m_Size   = size;

        if (!VulkanUtils::CreateRawBuffer(device, physicalDevice,
                             static_cast<VkDeviceSize>(size),
                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             m_Buffer, m_Memory))
        {
            LOG_ASSERT(false, "[VulkanUniformBuffer] Failed to create dynamic vertex buffer ({} bytes)", size);
            return false;
        }

        if (vkMapMemory(device, m_Memory, 0, static_cast<VkDeviceSize>(size), 0, &m_Mapped) != VK_SUCCESS)
        {
            LOG_ASSERT(false, "[VulkanUniformBuffer] Failed to map dynamic vertex buffer memory.");
            Destroy();
            return false;
        }

        LOG_INFO("[VulkanUniformBuffer] Created dynamic vertex buffer ({} bytes, persistently mapped)", size);
        return true;
    }

    void VulkanUniformBuffer::SetData(const void* data, std::size_t size, std::size_t offset)
    {
        if (m_Mapped && data && size > 0 && offset + size <= m_Size)
        {
            std::memcpy(static_cast<uint8_t*>(m_Mapped) + offset, data, size);
        }
        else
        {
            LOG_ERROR("[VulkanUniformBuffer] SetData out of bounds or buffer unmapped (size: {}, offset: {}, bufferSize: {})",
                      size, offset, m_Size);
        }
    }

    void VulkanUniformBuffer::Destroy()
    {
        if (m_Device == VK_NULL_HANDLE)
            return;

        if (m_Mapped != nullptr && m_Memory != VK_NULL_HANDLE)
        {
            vkUnmapMemory(m_Device, m_Memory);
            m_Mapped = nullptr;
        }

        if (m_Buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(m_Device, m_Buffer, nullptr);
            m_Buffer = VK_NULL_HANDLE;
        }
        if (m_Memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_Device, m_Memory, nullptr);
            m_Memory = VK_NULL_HANDLE;
        }
        m_Size = 0;
    }
}
