#pragma once

#include <Obscura/API.hpp>
#include <Obscura/IRHI.hpp>
#include "Scene.hpp"
#include "SceneGraph.hpp"
#include "Vulkan/VulkanBuffers.hpp"
#include "Vulkan/VulkanGraphicsPipeline.hpp"
#include "Vulkan/VulkanShader.hpp"

namespace Obscura
{
    // High-level scene renderer in Engine. Manages shader pipeline lifecycle,
    // quad geometry buffers, and frame render pass execution.
    class OBSCURA_ENGINE_API SceneRenderer
    {
    public:
        SceneRenderer() = default;
        ~SceneRenderer() { Shutdown(); }

        bool Initialize(IRHI* rhi);
        void Shutdown();

        // Renders the scene into the active frame render pass.
        void Render(const Scene& scene, IRHI* rhi);

        // Handles offscreen target resize.
        void OnResize(uint32_t width, uint32_t height, IRHI* rhi);

        [[nodiscard]] bool                    IsValid()    const noexcept { return m_Pipeline.IsValid(); }
        [[nodiscard]] VulkanGraphicsPipeline& GetPipeline()      noexcept { return m_Pipeline; }
        [[nodiscard]] VulkanVertexBuffer&     GetDefaultVB()      noexcept { return m_DefaultQuadVB; }
        [[nodiscard]] VulkanIndexBuffer&      GetDefaultIB()      noexcept { return m_DefaultQuadIB; }

    private:
        VulkanShader           m_VertShader;
        VulkanShader           m_FragShader;
        VulkanGraphicsPipeline m_Pipeline;
        VulkanVertexBuffer     m_DefaultQuadVB;
        VulkanIndexBuffer      m_DefaultQuadIB;
        SceneGraph             m_SceneGraph;

        bool                   m_Initialized = false;
    };
}
