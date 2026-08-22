#include <gtest/gtest.h>
#include <Vulkan/VulkanBindingSetManager.hpp>
#include <Vulkan/VulkanBindlessSystem.hpp>
#include <Vulkan/VulkanRHI.hpp>
#include <Vulkan/VulkanTexture.hpp>

class BindlessTextureTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_RHI = std::make_unique<Obscura::VulkanRHI>();
        ASSERT_TRUE(m_RHI->Initialize());
        m_RHI->SetRenderMode(Obscura::RenderMode::Offscreen);
        m_Device = m_RHI->GetDevice();
        m_PhysDevice = m_RHI->GetPhysicalDevice();
        m_Queue = m_RHI->GetGraphicsQueue();
        m_CmdPool = m_RHI->GetCommandPool();

        Obscura::VulkanBindlessSystem::Initialize(m_Device);
    }

    void TearDown() override
    {
        Obscura::VulkanBindlessSystem::Shutdown();
        Obscura::VulkanBindingSetManager::Shutdown();

        if (m_RHI)
        {
            m_RHI->Shutdown();
            m_RHI.reset();
        }
    }

    std::unique_ptr<Obscura::VulkanRHI> m_RHI;
    VkDevice         m_Device     = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysDevice = VK_NULL_HANDLE;
    VkQueue          m_Queue      = VK_NULL_HANDLE;
    VkCommandPool    m_CmdPool    = VK_NULL_HANDLE;
};

TEST_F(BindlessTextureTest, Texture_StagingUploadAndLifecycle)
{
    // 2x2 red RGBA pixel buffer
    const uint32_t pixels[4] = {
        0xFF0000FF, 0xFF0000FF,
        0xFF0000FF, 0xFF0000FF
    };

    Obscura::VulkanTexture texture;
    EXPECT_FALSE(texture.IsValid());

    bool created = texture.Create(m_Device, m_PhysDevice, m_CmdPool, m_Queue,
                                  2, 2, VK_FORMAT_R8G8B8A8_UNORM, pixels);
    EXPECT_TRUE(created);
    EXPECT_TRUE(texture.IsValid());
    EXPECT_EQ(texture.GetWidth(), 2u);
    EXPECT_EQ(texture.GetHeight(), 2u);
    EXPECT_NE(texture.GetImage(), VK_NULL_HANDLE);
    EXPECT_NE(texture.GetImageView(), VK_NULL_HANDLE);
    EXPECT_NE(texture.GetSampler(), VK_NULL_HANDLE);

    texture.Destroy();
    EXPECT_FALSE(texture.IsValid());
}

TEST_F(BindlessTextureTest, BindlessSystem_RegistrationAndRecycle)
{
    const uint32_t redPixel = 0xFF0000FF;
    Obscura::VulkanTexture tex1, tex2, tex3, tex4;
    ASSERT_TRUE(tex1.Create(m_Device, m_PhysDevice, m_CmdPool, m_Queue, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, &redPixel));
    ASSERT_TRUE(tex2.Create(m_Device, m_PhysDevice, m_CmdPool, m_Queue, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, &redPixel));
    ASSERT_TRUE(tex3.Create(m_Device, m_PhysDevice, m_CmdPool, m_Queue, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, &redPixel));
    ASSERT_TRUE(tex4.Create(m_Device, m_PhysDevice, m_CmdPool, m_Queue, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, &redPixel));

    uint32_t slot0 = Obscura::VulkanBindlessSystem::RegisterTexture(tex1);
    uint32_t slot1 = Obscura::VulkanBindlessSystem::RegisterTexture(tex2);
    uint32_t slot2 = Obscura::VulkanBindlessSystem::RegisterTexture(tex3);

    EXPECT_EQ(slot0, 0u);
    EXPECT_EQ(slot1, 1u);
    EXPECT_EQ(slot2, 2u);

    // Unregister slot1
    Obscura::VulkanBindlessSystem::UnregisterTexture(slot1);

    // Register tex4 - should recycle slot1
    uint32_t recycledSlot = Obscura::VulkanBindlessSystem::RegisterTexture(tex4);
    EXPECT_EQ(recycledSlot, 1u);
}

TEST_F(BindlessTextureTest, BindingSetManager_Caching)
{
    // Create dummy descriptor pool for testing binding cache
    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 10;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    ASSERT_EQ(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &pool), VK_SUCCESS);

    Obscura::VulkanBindingSetManager::Initialize(m_Device, pool);
    EXPECT_TRUE(Obscura::VulkanBindingSetManager::IsInitialized());

    // Create a descriptor set layout
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    ASSERT_EQ(vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &layout), VK_SUCCESS);

    const uint32_t redPixel = 0xFF0000FF;
    Obscura::VulkanTexture tex;
    ASSERT_TRUE(tex.Create(m_Device, m_PhysDevice, m_CmdPool, m_Queue, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, &redPixel));

    Obscura::DescriptorBindingSetDesc desc{};
    Obscura::DescriptorBindingItem item{};
    item.binding = 0;
    item.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    item.imageView = tex.GetImageView();
    item.sampler = tex.GetSampler();
    item.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    desc.bindings.push_back(item);

    VkDescriptorSet set1 = Obscura::VulkanBindingSetManager::GetOrCreateBindingSet(desc, layout);
    EXPECT_NE(set1, VK_NULL_HANDLE);

    VkDescriptorSet setCached = Obscura::VulkanBindingSetManager::GetCachedBindingSet(desc, layout);
    EXPECT_EQ(set1, setCached);

    VkDescriptorSet set2 = Obscura::VulkanBindingSetManager::GetOrCreateBindingSet(desc, layout);
    EXPECT_EQ(set1, set2);

    Obscura::VulkanBindingSetManager::Shutdown();
    vkDestroyDescriptorSetLayout(m_Device, layout, nullptr);
    vkDestroyDescriptorPool(m_Device, pool, nullptr);
}
