#pragma once

#include "Obscura/Logger.hpp"

#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstring>

#define VULKAN_CHECK(x, ...)                                                \
    do {                                                                    \
        if (x != VK_SUCCESS)                                                \
        {                                                                   \
            LOG_ERROR(__VA_ARGS__);                                         \
            LOG_ASSERT(false, "[Vulkan Assert] {} {}", __FILE__, __LINE__); \
        }                                                                   \
    } while (false)


namespace Obscura::VulkanUtils
{
    // -----------------------------------------------------------------------
    // Internal Helpers
    // -----------------------------------------------------------------------

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

    // Allocate a VkBuffer + DeviceMemory with the given usage and memory properties
    static bool CreateRawBuffer(VkDevice              device,
                                VkPhysicalDevice      physicalDevice,
                                VkDeviceSize          size,
                                VkBufferUsageFlags    usage,
                                VkMemoryPropertyFlags memProps,
                                VkBuffer              &outBuffer,
                                VkDeviceMemory        &outMemory)
    {
        // Create buffer
        VkBufferCreateInfo bufInfo = {};
        bufInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size        = size;
        bufInfo.usage       = usage;
        bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        const VkResult result = vkCreateBuffer(device, &bufInfo, nullptr, &outBuffer);
        VULKAN_CHECK(result, "Failed to create Buffer");
        if (result != VK_SUCCESS)
            return false;

        // Get memory requirements
        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device, outBuffer, &memReqs);

        // Create Device Memory
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memReqs.size;
        allocInfo.memoryTypeIndex = FindMemoryTypeIndex(physicalDevice,
                                                        memReqs.memoryTypeBits,
                                                        memProps);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &outMemory) != VK_SUCCESS)
        {
            vkDestroyBuffer(device, outBuffer, nullptr);
            outBuffer = VK_NULL_HANDLE;
            return false;
        }

        vkBindBufferMemory(device, outBuffer, outMemory, 0);
        return true;
    }

    // Copy 'size' bytes from stagingBuffer to dstBuffer using a one-shot command buffer
    static void CopyBuffer(VkDevice      device,
                           VkCommandPool commandPool,
                           VkQueue       queue,
                           VkBuffer      srcBuffer,
                           VkBuffer      dstBuffer,
                           VkDeviceSize  size)
    {
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool        = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &allocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        VkBufferCopy copyRegion = {};
        copyRegion.size = size;
        vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);

        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &cmd;
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
    }

    // Upload 'data' bytes into a device-local buffer of given usage, via staging buffer.
    static bool UploadToDeviceLocal(VkDevice         device,
                                    VkPhysicalDevice physicalDevice,
                                    VkCommandPool    commandPool,
                                    VkQueue          queue,
                                    const void       *data,
                                    std::size_t      size,
                                    VkBufferUsageFlags dstUsage,
                                    VkBuffer         &outBuffer,
                                    VkDeviceMemory   &outMemory)
    {
        // Create staging buffer (host-visible, host-coherent)
        VkBuffer stagingBuffer       = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

        if (!CreateRawBuffer(device, physicalDevice, static_cast<VkDeviceSize>(size),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingMemory))
        {
            return false;
        }

        // Update data to staging
        void *mapped = nullptr;
        vkMapMemory(device, stagingMemory, 0, static_cast<VkDeviceSize>(size), 0, &mapped);
        std::memcpy(mapped, data, size);
        vkUnmapMemory(device, stagingMemory);

        // Device local buffer
        if (!CreateRawBuffer(device, physicalDevice, static_cast<VkDeviceSize>(size),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | dstUsage,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outBuffer, outMemory))
        {
            vkDestroyBuffer(device, stagingBuffer, VK_NULL_HANDLE);
            vkFreeMemory(device, stagingMemory, VK_NULL_HANDLE);
            return false;
        }

        // Copy staging -> device
        CopyBuffer(device, commandPool, queue, stagingBuffer, outBuffer, static_cast<VkDeviceSize>(size));

        // Free staging
        vkDestroyBuffer(device, stagingBuffer, VK_NULL_HANDLE);
        vkFreeMemory(device, stagingMemory, VK_NULL_HANDLE);
        return true;
    }
}
