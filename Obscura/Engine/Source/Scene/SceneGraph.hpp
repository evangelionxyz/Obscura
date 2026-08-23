#pragma once

#include <Obscura/API.hpp>
#include <cstdint>

namespace Obscura
{
    class VulkanGraphicsPipeline;
    class VulkanVertexBuffer;
    class VulkanIndexBuffer;
    class Batch2D;
    class Scene;

    // Traverses Scene entities and records Vulkan draw commands.
    class OBSCURA_ENGINE_API SceneGraph
    {
    public:
        SceneGraph() = default;
        ~SceneGraph() = default;

        bool Begin(void *commandBuffer, Scene *scene, const uint32_t frameIndex = 0);
        void End();

        void RenderBatch2D(VulkanGraphicsPipeline *pipeline, Batch2D *batch2D = nullptr);

    private:
        void     *m_CommandBuffer = nullptr;
        Scene    *m_Scene         = nullptr;
        uint32_t m_FrameIndex     = 0;
    };
}

