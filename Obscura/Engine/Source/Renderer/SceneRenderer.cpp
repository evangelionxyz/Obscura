#include "SceneRenderer.hpp"

#include "Vulkan/VulkanBindlessSystem.hpp"
#include "Vulkan/VulkanBindingSetManager.hpp"
#include "Vulkan/VulkanUtils.hpp"

#include "Scene/Scene.hpp"

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
#include <cstring>
#include <algorithm>

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
        m_Device        = static_cast<VkDevice>(devObjects.device);
        m_PhysicalDevice= static_cast<VkPhysicalDevice>(devObjects.physicalDevice);
        auto queue      = static_cast<VkQueue>(devObjects.graphicsQueue);

        if (m_Device == VK_NULL_HANDLE)
        {
            LOG_ERROR("[SceneRenderer] Vulkan device is null in RHI.");
            return false;
        }

        // Initialize bindless system if not already initialized
        if (!VulkanBindlessSystem::IsInitialized())
        {
            VulkanBindlessSystem::Initialize(m_Device);
        }

        // 1. Resolve shader paths via VFS
        std::filesystem::path vertPath = VFS::ResolvePath("Resources/Shaders/default.vert.glsl");
        std::filesystem::path fragPath = VFS::ResolvePath("Resources/Shaders/default.frag.glsl");

        if (vertPath.empty() || fragPath.empty())
        {
            LOG_ASSERT(false, "[SceneRenderer] Failed to resolve default shaders via VFS.");
            return false;
        }

        LOG_INFO("[SceneRenderer] Resolved vertex shader: {}", vertPath.string());
        LOG_INFO("[SceneRenderer] Resolved fragment shader: {}", fragPath.string());

        // 2. Compile shaders
        if (!m_VertShader.Create(m_Device, vertPath, UMBRA_SHADER_TYPE_VERTEX))
        {
            LOG_ASSERT(false, "[SceneRenderer] Failed to compile vertex shader: {}", vertPath.string());
            return false;
        }

        if (!m_FragShader.Create(m_Device, fragPath, UMBRA_SHADER_TYPE_PIXEL))
        {
            LOG_ASSERT(false, "[SceneRenderer] Failed to compile fragment shader: {}", fragPath.string());
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
        {
            GraphicsPipelineCreateInfo pipelineInfo = {};
            Ref<VulkanGraphicsPipeline> pipeline = CreateRef<VulkanGraphicsPipeline>();
            if (!pipeline->Create(pipelineInfo, m_Device, m_VertShader, m_FragShader,
                                   format, ctx.width, ctx.height, imageViews,
                                   { VulkanBindlessSystem::GetDescriptorSetLayout() }))
            {
                LOG_ERROR("[SceneRenderer] Failed to create graphics pipeline.");
                return false;
            }

            m_Pipelines[PipelineType::BATCH_2D] = pipeline;
        }

        // Allocate command pool for staging copy and Batch2D initialization
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = devObjects.queueFamilyIndex;
        vkCreateCommandPool(m_Device, &poolInfo, nullptr, &commandPool);

        // Initialize Batch2D subsystem
        if (!m_Batch2D.Initialize(m_Device, m_PhysicalDevice, commandPool, queue))
        {
            LOG_WARN("[SceneRenderer] Failed to initialize Batch2D subsystem — falling back to per-quad rendering.");
        }

        vkDestroyCommandPool(m_Device, commandPool, nullptr);

        m_Initialized = true;
        LOG_INFO("[SceneRenderer] SceneRenderer initialized successfully.");
        return true;
    }

    VkDescriptorSet SceneRenderer::GetOrCreateCameraBindingSet(const ICamera* camera)
    {
        if (!camera || m_Device == VK_NULL_HANDLE)
        {
            return VK_NULL_HANDLE;
        }

        std::uintptr_t key = reinterpret_cast<std::uintptr_t>(camera);
        auto it = m_CameraCache.find(key);

        CameraUBO uboData {};
        uboData.viewProjection = camera->GetViewProjectionMatrix();
        uboData.view           = camera->GetViewMatrix();
        uboData.projection     = camera->GetProjectionMatrix();
        uboData.position       = glm::vec4(camera->GetPosition(), 1.0f);

        if (it != m_CameraCache.end())
        {
            if (it->second.uniformBuffer.GetMappedData())
            {
                it->second.uniformBuffer.SetData(&uboData, sizeof(CameraUBO));
            }
            return it->second.descriptorSet;
        }

        // Create UBO buffer for camera
        CameraCacheEntry entry{};
        entry.uniformBuffer.Create(m_Device, m_PhysicalDevice, sizeof(CameraUBO));
        entry.uniformBuffer.SetData(&uboData, sizeof(CameraUBO));

        if (VulkanBindingSetManager::IsInitialized() && VulkanBindlessSystem::GetDescriptorSetLayout() != VK_NULL_HANDLE)
        {
            DescriptorBindingSetDesc desc;
            DescriptorBindingItem item{};
            item.binding     = 0;
            item.type        = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            item.buffer      = entry.uniformBuffer.GetBuffer();
            item.offset      = 0;
            item.range       = sizeof(CameraUBO);
            desc.bindings.push_back(item);

            entry.descriptorSet = VulkanBindingSetManager::GetOrCreateBindingSet(desc, VulkanBindlessSystem::GetDescriptorSetLayout());
        }

        m_CameraCache[key] = entry;
        return entry.descriptorSet;
    }

    void SceneRenderer::ClearCameraCache()
    {
        for (auto& [key, entry] : m_CameraCache)
        {
            entry.uniformBuffer.Destroy();
        }
        m_CameraCache.clear();
    }

    void SceneRenderer::Render(Scene *scene, const ICamera& camera, IRHI* rhi)
    {
        const_cast<Scene&>(*scene).SetViewProj(camera.GetViewProjectionMatrix());
        GetOrCreateCameraBindingSet(&camera);
        Render(scene, rhi);
    }

    void SceneRenderer::Render(Scene *scene, IRHI* rhi)
    {
        auto batch2DPSO = GetPipeline(PipelineType::BATCH_2D);
        if (!batch2DPSO)
            return;

        if (!m_Initialized || !rhi)
        {
            return;
        }

        auto ctx = rhi->GetCurrentFrameContext();
        if (!ctx.commandBuffer || ctx.width == 0 || ctx.height == 0)
        {
            return;
        }

        // Lazy recreate framebuffers if not yet created, if size changed, or if offscreen image view handles changed
        std::array<VkImageView, PIPELINE_BACKBUFFER_COUNT> imageViews{};
        bool imageViewsChanged = false;
        for (uint32_t i = 0; i < PIPELINE_BACKBUFFER_COUNT; ++i)
        {
            imageViews[i] = static_cast<VkImageView>(const_cast<void*>(rhi->GetOffscreenImageView(i)));
            if (imageViews[i] != m_LastImageViews[i])
            {
                imageViewsChanged = true;
            }
        }

        if (!batch2DPSO->IsValid())
            return;

        if (batch2DPSO->GetFramebuffer(ctx.currentImageIndex) == VK_NULL_HANDLE ||
            batch2DPSO->GetWidth() != ctx.width ||
            batch2DPSO->GetHeight() != ctx.height ||
            imageViewsChanged)
        {
            if (imageViews[0] != VK_NULL_HANDLE)
            {
                batch2DPSO->RecreateFramebuffers(imageViews, ctx.width, ctx.height);
                m_LastImageViews = imageViews;
            }
        }

        VkFramebuffer fb = batch2DPSO->GetFramebuffer(ctx.currentImageIndex);
        if (fb == VK_NULL_HANDLE)
            return;

        auto cmd = static_cast<VkCommandBuffer>(ctx.commandBuffer);

        VkClearValue clearValue{};
        clearValue.color = { .float32 = { 0.1f, 0.1f, 0.1f, 1.0f } };

        VkRenderPassBeginInfo rpBegin{};
        rpBegin.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBegin.renderPass        = batch2DPSO->GetRenderPass();
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

        if (m_SceneGraph.Begin(cmd, scene, ctx.currentImageIndex))
        {
            m_SceneGraph.RenderBatch2D(batch2DPSO.get(), &m_Batch2D);
            m_SceneGraph.End();
        }

        vkCmdEndRenderPass(cmd);
    }

    void SceneRenderer::OnResize(uint32_t width, uint32_t height, IRHI* rhi)
    {
        if (!m_Initialized || !rhi)
        {
            return;
        }

        uint32_t actualW = rhi->GetFrameBufferWidth();
        uint32_t actualH = rhi->GetFrameBufferHeight();
        if (actualW == 0 || actualH == 0)
        {
            actualW = width;
            actualH = height;
        }

        std::array<VkImageView, PIPELINE_BACKBUFFER_COUNT> imageViews{};
        for (uint32_t i = 0; i < PIPELINE_BACKBUFFER_COUNT; ++i)
        {
            imageViews[i] = static_cast<VkImageView>(const_cast<void*>(rhi->GetOffscreenImageView(i)));
        }

        if (imageViews[0] != VK_NULL_HANDLE)
        {
            for (auto &pipeline : m_Pipelines | std::views::values)
            {
                pipeline->RecreateFramebuffers(imageViews, actualW, actualH);
            }
            m_LastImageViews = imageViews;
        }
    }


    Ref<VulkanGraphicsPipeline> SceneRenderer::GetPipeline(const PipelineType &type) noexcept
    {
        if (m_Pipelines.contains(type))
            return m_Pipelines[type];

        return nullptr;
    }

    void SceneRenderer::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        ClearCameraCache();
        m_Batch2D.Shutdown();

        for (auto &pipeline : m_Pipelines | std::views::values)
        {
            pipeline->Destroy();
        }
        m_Pipelines.clear();

        m_VertShader.Destroy();
        m_FragShader.Destroy();

        VulkanBindlessSystem::Shutdown();

        m_Device          = VK_NULL_HANDLE;
        m_PhysicalDevice  = VK_NULL_HANDLE;

        m_Initialized = false;
        LOG_INFO("[SceneRenderer] SceneRenderer shutdown complete.");
    }
}
