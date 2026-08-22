#include <gtest/gtest.h>
#include <Vulkan/VulkanRHI.hpp>

class RHIOffscreenTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_RHI = std::make_unique<Obscura::VulkanRHI>();
        ASSERT_TRUE(m_RHI->Initialize());
        m_RHI->SetRenderMode(Obscura::RenderMode::Offscreen);
    }

    void TearDown() override
    {
        if (m_RHI)
        {
            m_RHI->Shutdown();
            m_RHI.reset();
        }
    }

    std::unique_ptr<Obscura::VulkanRHI> m_RHI;
};

TEST_F(RHIOffscreenTest, VulkanRHI_Initialization)
{
    EXPECT_NE(m_RHI->GetInstance(), VK_NULL_HANDLE);
    EXPECT_NE(m_RHI->GetPhysicalDevice(), VK_NULL_HANDLE);
    EXPECT_NE(m_RHI->GetDevice(), VK_NULL_HANDLE);
    EXPECT_NE(m_RHI->GetGraphicsQueue(), VK_NULL_HANDLE);
    EXPECT_NE(m_RHI->GetCommandPool(), VK_NULL_HANDLE);
    EXPECT_TRUE(std::string(m_RHI->GetName()).find("Vulkan") != std::string::npos);
    EXPECT_TRUE(m_RHI->IsDescriptorIndexingSupported());
}

TEST_F(RHIOffscreenTest, VulkanRHI_OffscreenTargetCreationAndResize)
{
    EXPECT_TRUE(m_RHI->CreateOffscreenTarget(800, 600));
    EXPECT_EQ(m_RHI->GetFrameBufferWidth(), 800u);
    EXPECT_EQ(m_RHI->GetFrameBufferHeight(), 600u);
    EXPECT_EQ(m_RHI->GetFrameBufferStride(), 800u * 4u);
    EXPECT_NE(m_RHI->GetOffscreenImageView(0), nullptr);

    EXPECT_TRUE(m_RHI->ResizeOffscreenTarget(1024, 768));
    EXPECT_EQ(m_RHI->GetFrameBufferWidth(), 1024u);
    EXPECT_EQ(m_RHI->GetFrameBufferHeight(), 768u);
    EXPECT_EQ(m_RHI->GetFrameBufferStride(), 1024u * 4u);

    m_RHI->DestroyOffscreenTarget();
    EXPECT_EQ(m_RHI->GetFrameBufferWidth(), 0u);
    EXPECT_EQ(m_RHI->GetFrameBufferHeight(), 0u);
}

TEST_F(RHIOffscreenTest, VulkanRHI_TripleBufferedFrameExecution)
{
    ASSERT_TRUE(m_RHI->CreateOffscreenTarget(400, 300));

    // Execute multiple frames to exercise triple-buffering and fences
    for (int frame = 0; frame < 6; ++frame)
    {
        m_RHI->BeginFrame();
        auto ctx = m_RHI->GetCurrentFrameContext();
        EXPECT_NE(ctx.commandBuffer, nullptr);
        EXPECT_EQ(ctx.width, 400u);
        EXPECT_EQ(ctx.height, 300u);
        m_RHI->EndFrame();
    }
}
