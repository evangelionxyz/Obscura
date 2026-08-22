#include "VulkanBindingSetManager.hpp"

#include <Obscura/Logger.hpp>

#include <functional>
#include <shared_mutex>
#include <unordered_map>

namespace Obscura
{
    namespace
    {
        template <typename T>
        inline void HashCombine(size_t& seed, const T& val)
        {
            seed ^= std::hash<T>{}(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        size_t HashDescriptorBindingDesc(const DescriptorBindingSetDesc& desc, VkDescriptorSetLayout layout)
        {
            size_t seed = 0;
            HashCombine(seed, reinterpret_cast<uint64_t>(layout));
            HashCombine(seed, desc.bindings.size());

            for (const auto& b : desc.bindings)
            {
                HashCombine(seed, b.binding);
                HashCombine(seed, static_cast<uint32_t>(b.type));
                HashCombine(seed, reinterpret_cast<uint64_t>(b.buffer));
                HashCombine(seed, b.offset);
                HashCombine(seed, b.range);
                HashCombine(seed, reinterpret_cast<uint64_t>(b.imageView));
                HashCombine(seed, reinterpret_cast<uint64_t>(b.sampler));
                HashCombine(seed, static_cast<uint32_t>(b.imageLayout));
            }
            return seed;
        }

        struct CacheEntry
        {
            DescriptorBindingSetDesc desc;
            VkDescriptorSetLayout    layout = VK_NULL_HANDLE;
            VkDescriptorSet          set    = VK_NULL_HANDLE;
        };

        struct BindingCacheState
        {
            VkDevice                                  device         = VK_NULL_HANDLE;
            VkDescriptorPool                          descriptorPool = VK_NULL_HANDLE;
            std::unordered_map<size_t, CacheEntry>   cache;
            std::shared_mutex                         mutex;
            bool                                      initialized    = false;
        };

        BindingCacheState g_State;
    }

    void VulkanBindingSetManager::Initialize(VkDevice device, VkDescriptorPool descriptorPool)
    {
        std::unique_lock lock(g_State.mutex);
        g_State.device         = device;
        g_State.descriptorPool = descriptorPool;
        g_State.cache.clear();
        g_State.initialized    = (device != VK_NULL_HANDLE);
        LOG_INFO("[VulkanBindingSetManager] Initialized descriptor binding set manager.");
    }

    void VulkanBindingSetManager::Shutdown()
    {
        std::unique_lock lock(g_State.mutex);
        g_State.cache.clear();
        g_State.device         = VK_NULL_HANDLE;
        g_State.descriptorPool = VK_NULL_HANDLE;
        g_State.initialized    = false;
    }

    void VulkanBindingSetManager::Clear()
    {
        std::unique_lock lock(g_State.mutex);
        g_State.cache.clear();
    }

    bool VulkanBindingSetManager::IsInitialized() noexcept
    {
        std::shared_lock lock(g_State.mutex);
        return g_State.initialized;
    }

    VkDescriptorSet VulkanBindingSetManager::GetCachedBindingSet(const DescriptorBindingSetDesc& desc, VkDescriptorSetLayout layout)
    {
        if (layout == VK_NULL_HANDLE)
        {
            return VK_NULL_HANDLE;
        }

        size_t hash = HashDescriptorBindingDesc(desc, layout);

        std::shared_lock lock(g_State.mutex);
        auto it = g_State.cache.find(hash);
        if (it != g_State.cache.end() && it->second.layout == layout && it->second.desc == desc)
        {
            return it->second.set;
        }
        return VK_NULL_HANDLE;
    }

    VkDescriptorSet VulkanBindingSetManager::GetOrCreateBindingSet(const DescriptorBindingSetDesc& desc, VkDescriptorSetLayout layout)
    {
        if (layout == VK_NULL_HANDLE)
        {
            return VK_NULL_HANDLE;
        }

        size_t hash = HashDescriptorBindingDesc(desc, layout);

        {
            std::shared_lock readLock(g_State.mutex);
            auto it = g_State.cache.find(hash);
            if (it != g_State.cache.end() && it->second.layout == layout && it->second.desc == desc)
            {
                return it->second.set;
            }
        }

        std::unique_lock writeLock(g_State.mutex);

        // Double check after acquiring exclusive lock
        auto it = g_State.cache.find(hash);
        if (it != g_State.cache.end() && it->second.layout == layout && it->second.desc == desc)
        {
            return it->second.set;
        }

        if (g_State.device == VK_NULL_HANDLE || g_State.descriptorPool == VK_NULL_HANDLE)
        {
            LOG_ERROR("[VulkanBindingSetManager] Cannot allocate descriptor set - manager uninitialized.");
            return VK_NULL_HANDLE;
        }

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = g_State.descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &layout;

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        if (vkAllocateDescriptorSets(g_State.device, &allocInfo, &descriptorSet) != VK_SUCCESS)
        {
            LOG_ERROR("[VulkanBindingSetManager] vkAllocateDescriptorSets failed.");
            return VK_NULL_HANDLE;
        }

        // Apply descriptor writes for all items
        std::vector<VkWriteDescriptorSet> writes;
        std::vector<VkDescriptorImageInfo> imageInfos;
        std::vector<VkDescriptorBufferInfo> bufferInfos;
        writes.reserve(desc.bindings.size());
        imageInfos.reserve(desc.bindings.size());
        bufferInfos.reserve(desc.bindings.size());

        for (const auto& b : desc.bindings)
        {
            VkWriteDescriptorSet write{};
            write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet          = descriptorSet;
            write.dstBinding      = b.binding;
            write.dstArrayElement = 0;
            write.descriptorType  = b.type;
            write.descriptorCount = 1;

            if (b.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
                b.type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
                b.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
            {
                VkDescriptorImageInfo imgInfo{};
                imgInfo.sampler     = b.sampler;
                imgInfo.imageView   = b.imageView;
                imgInfo.imageLayout = b.imageLayout;
                imageInfos.push_back(imgInfo);
                write.pImageInfo = &imageInfos.back();
            }
            else if (b.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                     b.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            {
                VkDescriptorBufferInfo bufInfo{};
                bufInfo.buffer = b.buffer;
                bufInfo.offset = b.offset;
                bufInfo.range  = b.range;
                bufferInfos.push_back(bufInfo);
                write.pBufferInfo = &bufferInfos.back();
            }

            writes.push_back(write);
        }

        if (!writes.empty())
        {
            vkUpdateDescriptorSets(g_State.device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
        }

        CacheEntry entry{};
        entry.desc   = desc;
        entry.layout = layout;
        entry.set    = descriptorSet;

        g_State.cache[hash] = entry;
        return descriptorSet;
    }

    void VulkanBindingSetManager::RemoveBindingSet(const DescriptorBindingSetDesc& desc, VkDescriptorSetLayout layout)
    {
        size_t hash = HashDescriptorBindingDesc(desc, layout);

        std::unique_lock lock(g_State.mutex);
        g_State.cache.erase(hash);
    }
}
