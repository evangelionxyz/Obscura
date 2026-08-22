#pragma once

#include <Obscura/API.hpp>
#include "Scene.hpp"

namespace Obscura
{
    class VulkanGraphicsPipeline;
    class VulkanVertexBuffer;
    class VulkanIndexBuffer;

    // Traverses Scene entities and records Vulkan draw commands.
    class OBSCURA_ENGINE_API SceneGraph
    {
    public:
        SceneGraph() = default;
        ~SceneGraph() = default;

        // Records draw commands for all valid entities in 'scene' into 'commandBuffer'.
        // commandBuffer must be inside an active render pass.
        void Render(void* commandBuffer,
                    const Scene& scene,
                    VulkanGraphicsPipeline* pipeline,
                    VulkanVertexBuffer* quadVB,
                    VulkanIndexBuffer* quadIB);
    };
}
