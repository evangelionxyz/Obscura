#include "VulkanBindlessSystem.hpp"

#include <Obscura/Logger.hpp>

namespace Obscura
{
    VkDevice              VulkanBindlessSystem::s_Device              = VK_NULL_HANDLE;
    VkDescriptorPool      VulkanBindlessSystem::s_DescriptorPool      = VK_NULL_HANDLE;
    VkDescriptorSetLayout VulkanBindlessSystem::s_DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet       VulkanBindlessSystem::s_DescriptorSet       = VK_NULL_HANDLE;
    std::vector<uint32_t> VulkanBindlessSystem::s_FreeSlots;
    uint32_t              VulkanBindlessSystem::s_NextSlot            = 0;
    uint32_t              VulkanBindlessSystem::s_MaxTextures         = MAX_BINDLESS_TEXTURES;
    std::mutex            VulkanBindlessSystem::s_Mutex;
    bool                  VulkanBindlessSystem::s_Initialized         = false;

    bool VulkanBindlessSystem::Initialize(VkDevice device, uint32_t maxTextures)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        if (s_Initialized)
        {
            return true;
        }

        if (device == VK_NULL_HANDLE)
        {
            LOG_ERROR("[VulkanBindlessSystem] Invalid VkDevice provided.");
            return false;
        }

        s_Device      = device;
        s_MaxTextures = maxTextures;
        s_NextSlot    = 0;
        s_FreeSlots.clear();

        // 1. Create Descriptor Set Layout with update-after-bind and partially-bound flags
        VkDescriptorSetLayoutBinding binding{};
        binding.binding         = 0;
        binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = s_MaxTextures;
        binding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorBindingFlags bindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
                                                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

        VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
        flagsInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        flagsInfo.bindingCount  = 1;
        flagsInfo.pBindingFlags = &bindingFlags;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.pNext        = &flagsInfo;
        layoutInfo.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings    = &binding;

        if (vkCreateDescriptorSetLayout(s_Device, &layoutInfo, nullptr, &s_DescriptorSetLayout) != VK_SUCCESS)
        {
            LOG_ERROR("[VulkanBindlessSystem] Failed to create bindless descriptor set layout.");
            Shutdown();
            return false;
        }

        // 2. Create Descriptor Pool
        VkDescriptorPoolSize poolSize{};
        poolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = s_MaxTextures;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        poolInfo.maxSets       = 1;
        poolSize.descriptorCount = s_MaxTextures;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes    = &poolSize;

        if (vkCreateDescriptorPool(s_Device, &poolInfo, nullptr, &s_DescriptorPool) != VK_SUCCESS)
        {
            LOG_ERROR("[VulkanBindlessSystem] Failed to create bindless descriptor pool.");
            Shutdown();
            return false;
        }

        // 3. Allocate Global Descriptor Set
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = s_DescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &s_DescriptorSetLayout;

        if (vkAllocateDescriptorSets(s_Device, &allocInfo, &s_DescriptorSet) != VK_SUCCESS)
        {
            LOG_ERROR("[VulkanBindlessSystem] Failed to allocate bindless descriptor set.");
            Shutdown();
            return false;
        }

        s_Initialized = true;
        LOG_INFO("[VulkanBindlessSystem] Initialized with {} maximum bindless texture slots.", s_MaxTextures);
        return true;
    }

    void VulkanBindlessSystem::Shutdown()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        if (s_Device != VK_NULL_HANDLE)
        {
            if (s_DescriptorPool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(s_Device, s_DescriptorPool, nullptr);
                s_DescriptorPool = VK_NULL_HANDLE;
            }
            if (s_DescriptorSetLayout != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorSetLayout(s_Device, s_DescriptorSetLayout, nullptr);
                s_DescriptorSetLayout = VK_NULL_HANDLE;
            }
        }

        s_DescriptorSet = VK_NULL_HANDLE;
        s_Device        = VK_NULL_HANDLE;
        s_NextSlot      = 0;
        s_FreeSlots.clear();
        s_Initialized   = false;
    }

    uint32_t VulkanBindlessSystem::RegisterTexture(const VulkanTexture& texture)
    {
        if (!texture.IsValid())
        {
            return INVALID_BINDLESS_INDEX;
        }

        return RegisterTexture(texture.GetImageView(), texture.GetSampler());
    }

    uint32_t VulkanBindlessSystem::RegisterTexture(VkImageView imageView, VkSampler sampler)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        if (!s_Initialized || imageView == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE)
        {
            return INVALID_BINDLESS_INDEX;
        }

        uint32_t slot = INVALID_BINDLESS_INDEX;

        if (!s_FreeSlots.empty())
        {
            slot = s_FreeSlots.back();
            s_FreeSlots.pop_back();
        }
        else if (s_NextSlot < s_MaxTextures)
        {
            slot = s_NextSlot++;
        }
        else
        {
            LOG_ERROR("[VulkanBindlessSystem] Out of bindless texture slots (max: {}).", s_MaxTextures);
            return INVALID_BINDLESS_INDEX;
        }

        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler     = sampler;
        imageInfo.imageView   = imageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = s_DescriptorSet;
        write.dstBinding      = 0;
        write.dstArrayElement = slot;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo      = &imageInfo;

        vkUpdateDescriptorSets(s_Device, 1, &write, 0, nullptr);
        return slot;
    }

    void VulkanBindlessSystem::UnregisterTexture(uint32_t slot)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        if (!s_Initialized || slot >= s_MaxTextures)
        {
            return;
        }

        s_FreeSlots.push_back(slot);
    }
}
