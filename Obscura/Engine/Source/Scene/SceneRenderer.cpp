#include "SceneRenderer.hpp"

#include "Vulkan/VulkanBindlessSystem.hpp"
#include <Obscura/Logger.hpp>
#include <Obscura/VFS.hpp>

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
#include <vector>

namespace Obscura
{
    bool SceneRenderer::Initialize(IRHI* rhi)
    {
        if (!rhi)
        {
            LOG_ERROR("[SceneRenderer] Invalid IRHI pointer passed to Initialize.");
            return false;
        }

        LOG_INFO("[SceneRenderer] Initializing SceneRenderer subsystem...");

        auto devObjects = rhi->GetVulkanDeviceObjects();
        auto device     = static_cast<VkDevice>(devObjects.device);
        auto physDevice = static_cast<VkPhysicalDevice>(devObjects.physicalDevice);
        auto queue      = static_cast<VkQueue>(devObjects.graphicsQueue);

        if (device == VK_NULL_HANDLE)
        {
            LOG_ERROR("[SceneRenderer] Vulkan device is null in RHI.");
            return false;
        }

        // Initialize bindless system if not already initialized
        if (!VulkanBindlessSystem::IsInitialized())
        {
            VulkanBindlessSystem::Initialize(device);
        }

        // 1. Resolve shader paths via VFS
        std::filesystem::path vertPath = VFS::ResolvePath("Resources/Shaders/default.vert.glsl");
        if (vertPath.empty())
        {
            vertPath = VFS::ResolvePath("Shaders/default.vert.glsl");
        }
        if (vertPath.empty())
        {
            vertPath = VFS::ResolvePath("default.vert.glsl");
        }

        std::filesystem::path fragPath = VFS::ResolvePath("Resources/Shaders/default.frag.glsl");
        if (fragPath.empty())
        {
            fragPath = VFS::ResolvePath("Shaders/default.frag.glsl");
        }
        if (fragPath.empty())
        {
            fragPath = VFS::ResolvePath("default.frag.glsl");
        }

        if (vertPath.empty() || fragPath.empty())
        {
            LOG_ERROR("[SceneRenderer] Failed to resolve default shaders via VFS.");
            return false;
        }

        LOG_INFO("[SceneRenderer] Resolved vertex shader: {}", vertPath.string());
        LOG_INFO("[SceneRenderer] Resolved fragment shader: {}", fragPath.string());

        // 2. Compile shaders
        if (!m_VertShader.Create(device, vertPath, UMBRA_SHADER_TYPE_VERTEX))
        {
            LOG_ERROR("[SceneRenderer] Failed to compile vertex shader: {}", vertPath.string());
            return false;
        }

        if (!m_FragShader.Create(device, fragPath, UMBRA_SHADER_TYPE_PIXEL))
        {
            LOG_ERROR("[SceneRenderer] Failed to compile fragment shader: {}", fragPath.string());
            return false;
        }

        // 3. Collect offscreen image views from RHI
        std::array<VkImageView, PIPELINE_BACKBUFFER_COUNT> imageViews{};
        for (uint32_t i = 0; i < PIPELINE_BACKBUFFER_COUNT; ++i)
        {
            imageViews[i] = static_cast<VkImageView>(const_cast<void*>(rhi->GetOffscreenImageView(i)));
        }

        auto ctx = rhi->GetCurrentFrameContext();
        auto format = static_cast<VkFormat>(rhi->GetOffscreenFormat());

        // 4. Create graphics pipeline with bindless descriptor set layout
        std::vector<VkDescriptorSetLayout> externalLayouts;
        if (VulkanBindlessSystem::IsInitialized() && VulkanBindlessSystem::GetDescriptorSetLayout() != VK_NULL_HANDLE)
        {
            externalLayouts.push_back(VulkanBindlessSystem::GetDescriptorSetLayout());
        }

        if (!m_Pipeline.Create(device, m_VertShader, m_FragShader,
                                format, ctx.width, ctx.height, imageViews, externalLayouts))
        {
            LOG_ERROR("[SceneRenderer] Failed to create graphics pipeline.");
            return false;
        }

        // 5. Upload default unit quad mesh (4 vertices, 6 indices)
        struct Vertex
        {
            float pos[3];
            float uv[2];
            float col[4];
        };

        const Vertex vertices[] = {
            { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }, // bottom-left
            { {  0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }, // bottom-right
            { {  0.5f,  0.5f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }, // top-right
            { { -0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }, // top-left
        };
        const uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };

        // Allocate command buffer for staging copy
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = devObjects.queueFamilyIndex;
        vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);

        if (!m_DefaultQuadVB.Create(device, physDevice, commandPool, queue,
                                    vertices, sizeof(vertices)))
        {
            LOG_ERROR("[SceneRenderer] Failed to create quad vertex buffer.");
            vkDestroyCommandPool(device, commandPool, nullptr);
            return false;
        }

        if (!m_DefaultQuadIB.Create(device, physDevice, commandPool, queue,
                                    indices, sizeof(indices), 6, VK_INDEX_TYPE_UINT32))
        {
            LOG_ERROR("[SceneRenderer] Failed to create quad index buffer.");
            vkDestroyCommandPool(device, commandPool, nullptr);
            return false;
        }

        // 6. Pre-load available textures into Vulkan Bindless System
        m_LoadedTextures.clear();

        std::vector<std::filesystem::path> textureFiles;
        std::filesystem::path texFileProbe = VFS::ResolvePath("Resources/Textures/img_1.jpg");
        std::filesystem::path texDir;
        if (!texFileProbe.empty())
        {
            texDir = texFileProbe.parent_path();
        }
        else
        {
            texFileProbe = VFS::ResolvePath("Textures/img_1.jpg");
            if (!texFileProbe.empty())
            {
                texDir = texFileProbe.parent_path();
            }
        }

        if (!texDir.empty() && std::filesystem::exists(texDir) && std::filesystem::is_directory(texDir))
        {
            for (const auto& entry : std::filesystem::directory_iterator(texDir))
            {
                if (entry.is_regular_file())
                {
                    auto ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".tga" || ext == ".bmp")
                    {
                        textureFiles.push_back(entry.path());
                    }
                }
            }
            std::sort(textureFiles.begin(), textureFiles.end());
        }

        if (textureFiles.empty())
        {
            for (const auto& relPath : { "Resources/Textures/img_1.jpg", "Resources/Textures/img_2.jpg", "Resources/Textures/img_3.jpg" })
            {
                auto p = VFS::ResolvePath(relPath);
                if (!p.empty() && std::filesystem::exists(p))
                {
                    textureFiles.push_back(p);
                }
            }
        }

        for (const auto& filePath : textureFiles)
        {
            auto tex = std::make_unique<VulkanTexture>();
            if (tex->LoadFromFile(device, physDevice, commandPool, queue, filePath))
            {
                uint32_t slot = VulkanBindlessSystem::RegisterTexture(*tex);
                LOG_INFO("[SceneRenderer] Loaded texture '{}' -> Bindless Slot {}", filePath.filename().string(), slot);
                m_LoadedTextures.push_back(std::move(tex));
            }
            else
            {
                LOG_WARN("[SceneRenderer] Failed to load texture '{}'", filePath.string());
            }
        }

        if (m_LoadedTextures.empty())
        {
            uint32_t whitePixel = 0xFFFFFFFF;
            auto whiteTex = std::make_unique<VulkanTexture>();
            if (whiteTex->Create(device, physDevice, commandPool, queue, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, &whitePixel))
            {
                uint32_t slot = VulkanBindlessSystem::RegisterTexture(*whiteTex);
                LOG_INFO("[SceneRenderer] Created fallback 1x1 white texture -> Bindless Slot {}", slot);
                m_LoadedTextures.push_back(std::move(whiteTex));
            }
        }

        vkDestroyCommandPool(device, commandPool, nullptr);

        m_Initialized = true;
        LOG_INFO("[SceneRenderer] SceneRenderer initialized successfully.");
        return true;
    }

    void SceneRenderer::Render(const Scene& scene, IRHI* rhi)
    {
        if (!m_Initialized || !rhi || !m_Pipeline.IsValid())
        {
            return;
        }

        auto ctx = rhi->GetCurrentFrameContext();
        if (!ctx.commandBuffer || ctx.width == 0 || ctx.height == 0)
        {
            return;
        }

        // Lazy recreate framebuffers if not yet created or if size changed
        if (m_Pipeline.GetFramebuffer(ctx.currentImageIndex) == VK_NULL_HANDLE ||
            m_Pipeline.GetWidth() != ctx.width || m_Pipeline.GetHeight() != ctx.height)
        {
            std::array<VkImageView, PIPELINE_BACKBUFFER_COUNT> imageViews{};
            for (uint32_t i = 0; i < PIPELINE_BACKBUFFER_COUNT; ++i)
            {
                imageViews[i] = static_cast<VkImageView>(const_cast<void*>(rhi->GetOffscreenImageView(i)));
            }
            if (imageViews[0] != VK_NULL_HANDLE)
            {
                m_Pipeline.RecreateFramebuffers(imageViews, ctx.width, ctx.height);
            }
        }

        VkFramebuffer fb = m_Pipeline.GetFramebuffer(ctx.currentImageIndex);
        if (fb == VK_NULL_HANDLE)
        {
            return;
        }

        auto cmd = static_cast<VkCommandBuffer>(ctx.commandBuffer);

        VkClearValue clearValue{};
        clearValue.color = { .float32 = { 0.1f, 0.1f, 0.1f, 1.0f } };

        VkRenderPassBeginInfo rpBegin{};
        rpBegin.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBegin.renderPass        = m_Pipeline.GetRenderPass();
        rpBegin.framebuffer       = fb;
        rpBegin.renderArea.offset = { 0, 0 };
        rpBegin.renderArea.extent = { ctx.width, ctx.height };
        rpBegin.clearValueCount   = 1;
        rpBegin.pClearValues      = &clearValue;

        vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

        // Dynamic viewport and scissor
        VkViewport viewport{};
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = static_cast<float>(ctx.width);
        viewport.height   = static_cast<float>(ctx.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = { ctx.width, ctx.height };
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        m_SceneGraph.Render(cmd, scene, &m_Pipeline, &m_DefaultQuadVB, &m_DefaultQuadIB);

        vkCmdEndRenderPass(cmd);
    }

    void SceneRenderer::OnResize(uint32_t width, uint32_t height, IRHI* rhi)
    {
        if (!m_Initialized || !rhi || !m_Pipeline.IsValid())
        {
            return;
        }

        std::array<VkImageView, PIPELINE_BACKBUFFER_COUNT> imageViews{};
        for (uint32_t i = 0; i < PIPELINE_BACKBUFFER_COUNT; ++i)
        {
            imageViews[i] = static_cast<VkImageView>(const_cast<void*>(rhi->GetOffscreenImageView(i)));
        }

        m_Pipeline.RecreateFramebuffers(imageViews, width, height);
    }

    void SceneRenderer::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        m_LoadedTextures.clear();
        m_DefaultQuadVB.Destroy();
        m_DefaultQuadIB.Destroy();
        m_Pipeline.Destroy();
        m_VertShader.Destroy();
        m_FragShader.Destroy();
        VulkanBindlessSystem::Shutdown();

        m_Initialized = false;
        LOG_INFO("[SceneRenderer] SceneRenderer shutdown complete.");
    }
}
