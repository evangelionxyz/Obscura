#include "SceneGraph.hpp"

#include "Vulkan/VulkanBindlessSystem.hpp"
#include "Vulkan/VulkanBuffers.hpp"
#include "Vulkan/VulkanGraphicsPipeline.hpp"
#include "Renderer/Batch2D.hpp"

#include "Scene/Scene.hpp"

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

namespace Obscura
{
    void SceneGraph::RenderBatch2D(VulkanGraphicsPipeline *pipeline, Batch2D *batch2D)
    {
        auto cmd = static_cast<VkCommandBuffer>(m_CommandBuffer);

        // Batch 2D Pass
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

        // 2Bind global bindless descriptor set (Set 0) if available
        if (VulkanBindlessSystem::IsInitialized() && VulkanBindlessSystem::GetDescriptorSet() != VK_NULL_HANDLE)
        {
            VkDescriptorSet bindlessSet = VulkanBindlessSystem::GetDescriptorSet();
            vkCmdBindDescriptorSets(cmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipeline->GetLayout(),
                                    0,
                                    1,
                                    &bindlessSet,
                                    0,
                                    nullptr);
        }

        if (batch2D && batch2D->IsInitialized())
        {
            batch2D->BeginBatch(m_Scene->GetViewProj(), m_FrameIndex);

            const auto& registry = m_Scene->GetRegistry();
            const auto spriteView = registry.view<Transform, Sprite2D>();

            for (auto entity : spriteView)
            {
                const auto& [transform, sprite] = spriteView.get<Transform, Sprite2D>(entity);
                if (!sprite.visible)
                {
                    continue;
                }

                uint32_t useTex = sprite.useTexture ? 1u : 0u;
                if (batch2D->GetQuadCount() > 0 && (batch2D->GetCurrentTextureSlot() != sprite.textureSlot || batch2D->GetCurrentUseTexture() != useTex))
                {
                    batch2D->Flush(cmd, pipeline->GetLayout());
                    batch2D->BeginBatch(m_Scene->GetViewProj(), m_FrameIndex);
                }

                batch2D->DrawQuad(transform.GetTransformMatrix(), sprite.color, sprite.textureSlot, useTex, sprite.uvOffset, sprite.uvScale);
            }

            if (batch2D->GetQuadCount() > 0)
            {
                batch2D->Flush(cmd, pipeline->GetLayout());
                batch2D->EndBatch();
            }
        }
    }

    bool SceneGraph::Begin(void *commandBuffer, Scene *scene, const uint32_t frameIndex)
    {
        if (!commandBuffer || !scene)
            return false;

        m_CommandBuffer = commandBuffer;
        m_Scene = scene;
        m_FrameIndex = frameIndex;

        return true;
    }

    void SceneGraph::End()
    {

    }

}
