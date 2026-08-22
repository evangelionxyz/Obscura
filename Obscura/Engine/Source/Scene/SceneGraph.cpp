#include "SceneGraph.hpp"

#include "Vulkan/VulkanBindlessSystem.hpp"
#include "Vulkan/VulkanBuffers.hpp"
#include "Vulkan/VulkanGraphicsPipeline.hpp"

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
    void SceneGraph::Render(void* commandBufferVoid,
                            const Scene& scene,
                            VulkanGraphicsPipeline* pipeline,
                            VulkanVertexBuffer* quadVB,
                            VulkanIndexBuffer* quadIB)
    {
        if (!commandBufferVoid || !pipeline || !pipeline->IsValid() || !quadVB || !quadIB)
        {
            return;
        }

        auto cmd = static_cast<VkCommandBuffer>(commandBufferVoid);

        // 1. Bind Graphics Pipeline
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

        // 2. Bind global bindless descriptor set (Set 0) if available
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

        // 3. Bind Quad Vertex and Index Buffers
        VkBuffer     vertBufs[] = { quadVB->GetBuffer() };
        VkDeviceSize offsets[]  = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, vertBufs, offsets);
        vkCmdBindIndexBuffer(cmd, quadIB->GetBuffer(), 0, quadIB->GetIndexType());

        // 4. Iterate and render all Sprite2D entities
        const auto& registry = scene.GetRegistry();
        const auto spriteView = registry.view<Transform, Sprite2D>();

        for (auto entity : spriteView)
        {
            const auto& [transform, sprite] = spriteView.get<Transform, Sprite2D>(entity);
            if (!sprite.visible)
            {
                continue;
            }

            SpritePushConstants pushConstants{};
            pushConstants.model       = scene.GetViewProj() * transform.GetTransformMatrix();
            pushConstants.color       = sprite.color;
            pushConstants.uvOffset    = sprite.uvOffset;
            pushConstants.uvScale     = sprite.uvScale;
            pushConstants.textureSlot = sprite.textureSlot;
            pushConstants.useTexture  = sprite.useTexture ? 1u : 0u;

            vkCmdPushConstants(cmd,
                               pipeline->GetLayout(),
                               VK_SHADER_STAGE_VERTEX_BIT,
                               0,
                               sizeof(SpritePushConstants),
                               &pushConstants);

            vkCmdDrawIndexed(cmd, quadIB->GetIndexCount(), 1, 0, 0, 0);
        }
    }
}
