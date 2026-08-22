#include <gtest/gtest.h>
#include <Scene.hpp>
#include <SceneRenderer.hpp>
#include <Vulkan/VulkanBindlessSystem.hpp>
#include <Vulkan/VulkanRHI.hpp>
#include <Vulkan/VulkanTexture.hpp>

class SceneSpriteRenderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_RHI = std::make_unique<Obscura::VulkanRHI>();
        ASSERT_TRUE(m_RHI->Initialize());
        m_RHI->SetRenderMode(Obscura::RenderMode::Offscreen);
        ASSERT_TRUE(m_RHI->CreateOffscreenTarget(512, 512));
    }

    void TearDown() override
    {
        if (m_RHI)
        {
            auto devObjects = m_RHI->GetVulkanDeviceObjects();
            if (devObjects.graphicsQueue)
            {
                vkQueueWaitIdle(static_cast<VkQueue>(devObjects.graphicsQueue));
            }
        }

        if (m_SceneRenderer)
        {
            m_SceneRenderer->Shutdown();
            m_SceneRenderer.reset();
        }

        Obscura::VulkanBindlessSystem::Shutdown();

        if (m_RHI)
        {
            m_RHI->Shutdown();
            m_RHI.reset();
        }
    }

    std::unique_ptr<Obscura::VulkanRHI>     m_RHI;
    std::unique_ptr<Obscura::SceneRenderer> m_SceneRenderer;
};

TEST_F(SceneSpriteRenderTest, SceneRenderer_InitializeAndOffscreenSpritePass)
{
    m_SceneRenderer = std::make_unique<Obscura::SceneRenderer>();
    ASSERT_TRUE(m_SceneRenderer->Initialize(m_RHI.get()));
    EXPECT_TRUE(m_SceneRenderer->IsValid());

    // Create Scene with two entities
    Obscura::Scene scene;
    
    // Entity 1: Solid green sprite quad
    auto greenEntity = scene.CreateEntity("GreenSprite");
    auto& greenTransform = scene.GetComponent<Obscura::Transform>(greenEntity);
    greenTransform.position = glm::vec3(-0.3f, 0.0f, 0.0f);
    greenTransform.scale    = glm::vec3(0.4f, 0.4f, 1.0f);

    auto& greenSprite = scene.AddComponent<Obscura::Sprite2D>(greenEntity);
    greenSprite.color      = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    greenSprite.useTexture = false;

    // Entity 2: Textured blue sprite quad
    const uint32_t bluePixel = 0xFFFF0000; // RGBA Blue
    Obscura::VulkanTexture blueTex;
    auto devObjects = m_RHI->GetVulkanDeviceObjects();
    ASSERT_TRUE(blueTex.Create(static_cast<VkDevice>(devObjects.device),
                               static_cast<VkPhysicalDevice>(devObjects.physicalDevice),
                               static_cast<VkCommandPool>(m_RHI->GetCommandPool()),
                               static_cast<VkQueue>(devObjects.graphicsQueue),
                               1, 1, VK_FORMAT_R8G8B8A8_UNORM, &bluePixel));

    uint32_t blueSlot = Obscura::VulkanBindlessSystem::RegisterTexture(blueTex);
    EXPECT_NE(blueSlot, Obscura::INVALID_BINDLESS_INDEX);

    auto blueEntity = scene.CreateEntity("BlueSprite");
    auto& blueTransform = scene.GetComponent<Obscura::Transform>(blueEntity);
    blueTransform.position = glm::vec3(0.3f, 0.0f, 0.0f);
    blueTransform.scale    = glm::vec3(0.4f, 0.4f, 1.0f);

    auto& blueSprite = scene.AddComponent<Obscura::Sprite2D>(blueEntity);
    blueSprite.color       = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    blueSprite.textureSlot = blueSlot;
    blueSprite.useTexture  = true;

    // Render offscreen frame
    for (int frame = 0; frame < 3; ++frame)
    {
        m_RHI->BeginFrame();
        m_SceneRenderer->Render(scene, m_RHI.get());
        m_RHI->EndFrame();
    }

    // Verify resize works
    m_RHI->ResizeOffscreenTarget(800, 600);
    m_SceneRenderer->OnResize(800, 600, m_RHI.get());

    m_RHI->BeginFrame();
    m_SceneRenderer->Render(scene, m_RHI.get());
    m_RHI->EndFrame();

    vkQueueWaitIdle(static_cast<VkQueue>(devObjects.graphicsQueue));
    blueTex.Destroy();
}
