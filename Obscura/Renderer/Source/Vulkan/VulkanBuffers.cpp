#include "VulkanBuffers.hpp"

#include <Obscura/Logger.hpp>

#include <cstring>

namespace Obscura
{
    // ---------------------------------------------------------------------------
    // Internal helpers
    // ---------------------------------------------------------------------------
    static uint32_t FindMemoryTypeIndex(VkPhysicalDevice physicalDevice,
                                        uint32_t         typeFilter,
                                        VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1u << i)) &&
                (memProps.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        // Fallback: first matching type bit regardless of property flags
        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
        {
            if (typeFilter & (1u << i))
                return i;
        }
        return 0;
    }

    // Allocate a VkBuffer + VkDeviceMemory with the given usage and memory properties.
    static bool CreateRawBuffer(VkDevice              device,
                                VkPhysicalDevice      physicalDevice,
                                VkDeviceSize          size,
                                VkBufferUsageFlags    usage,
                                VkMemoryPropertyFlags memProps,
                                VkBuffer&             outBuffer,
                                VkDeviceMemory&       outMemory)
    {
        VkBufferCreateInfo bufInfo{};
        bufInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size        = size;
        bufInfo.usage       = usage;
        bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufInfo, nullptr, &outBuffer) != VK_SUCCESS)
            return false;

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device, outBuffer, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memReqs.size;
        allocInfo.memoryTypeIndex = FindMemoryTypeIndex(physicalDevice, memReqs.memoryTypeBits, memProps);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &outMemory) != VK_SUCCESS)
        {
            vkDestroyBuffer(device, outBuffer, nullptr);
            outBuffer = VK_NULL_HANDLE;
            return false;
        }

        vkBindBufferMemory(device, outBuffer, outMemory, 0);
        return true;
    }

    // Copy 'size' bytes from stagingBuffer to dstBuffer using a one-shot command buffer.
    static void CopyBuffer(VkDevice      device,
                           VkCommandPool commandPool,
                           VkQueue       queue,
                           VkBuffer      srcBuffer,
                           VkBuffer      dstBuffer,
                           VkDeviceSize  size)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool        = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &allocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);

        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &cmd;
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
    }

    // Upload 'data' bytes into a device-local buffer of given usage, via a staging buffer.
    static bool UploadToDeviceLocal(VkDevice          device,
                                    VkPhysicalDevice  physicalDevice,
                                    VkCommandPool     commandPool,
                                    VkQueue           queue,
                                    const void*       data,
                                    std::size_t       size,
                                    VkBufferUsageFlags dstUsage,
                                    VkBuffer&         outBuffer,
                                    VkDeviceMemory&   outMemory)
    {
        // 1. Staging buffer (host-visible, host-coherent)
        VkBuffer       stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

        if (!CreateRawBuffer(device, physicalDevice,
                             static_cast<VkDeviceSize>(size),
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             stagingBuffer, stagingMemory))
        {
            return false;
        }

        // 2. Upload data to staging
        void* mapped = nullptr;
        vkMapMemory(device, stagingMemory, 0, static_cast<VkDeviceSize>(size), 0, &mapped);
        std::memcpy(mapped, data, size);
        vkUnmapMemory(device, stagingMemory);

        // 3. Device-local buffer
        if (!CreateRawBuffer(device, physicalDevice,
                             static_cast<VkDeviceSize>(size),
                             VK_BUFFER_USAGE_TRANSFER_DST_BIT | dstUsage,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             outBuffer, outMemory))
        {
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingMemory, nullptr);
            return false;
        }

        // 4. Copy staging -> device
        CopyBuffer(device, commandPool, queue, stagingBuffer, outBuffer, static_cast<VkDeviceSize>(size));

        // 5. Free staging
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return true;
    }

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

        bool ok = UploadToDeviceLocal(device, physicalDevice, commandPool, queue,
                                      data, size,
                                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                      m_Buffer, m_Memory);
        if (!ok)
        {
            LOG_ERROR("[VulkanVertexBuffer] Failed to create device-local vertex buffer ({} bytes)", size);
        }
        else
        {
            LOG_INFO("[VulkanVertexBuffer] Created vertex buffer ({} bytes)", size);
        }
        return ok;
    }

    void VulkanVertexBuffer::Destroy()
    {
        if (m_Device == VK_NULL_HANDLE)
            return;
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

        bool ok = UploadToDeviceLocal(device, physicalDevice, commandPool, queue,
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

    void VulkanIndexBuffer::Destroy()
    {
        if (m_Device == VK_NULL_HANDLE)
            return;
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
}
