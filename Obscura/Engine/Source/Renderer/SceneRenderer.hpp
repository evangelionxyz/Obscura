#pragma once

#include <Obscura/API.hpp>
#include <Obscura/IRHI.hpp>

#include "Scene/Scene.hpp"
#include "Scene/SceneGraph.hpp"
#include "Scene/ICamera.hpp"
#include "Scene/SceneCamera.hpp"
#include "Scene/EditorCamera.hpp"
#include "Renderer/Batch2D.hpp"
#include "Vulkan/VulkanBuffers.hpp"
#include "Vulkan/VulkanGraphicsPipeline.hpp"
#include "Vulkan/VulkanShader.hpp"
#include "Vulkan/VulkanTexture.hpp"

#include <memory>
#include <unordered_map>
#include <map>
#include <vector>
#include <cstdint>

namespace Obscura
{
    struct CameraUBO
    {
        glm::mat4 viewProjection = glm::mat4(1.0f);
        glm::mat4 view           = glm::mat4(1.0f);
        glm::mat4 projection     = glm::mat4(1.0f);
        glm::vec4 position       = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    };

    struct CameraCacheEntry
    {
        VulkanUniformBuffer uniformBuffer    = {};
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };

    enum class PipelineType
    {
        BATCH_2D = 0,
        STATIC_MESH,
        ANIMATED_MESH,

        CSM_STATIC_MESH,
        CSM_ANIMATED_MESH,
    };

    class OBSCURA_ENGINE_API SceneRenderer
    {
    public:
        SceneRenderer() = default;
        ~SceneRenderer() { Shutdown(); }

        SceneRenderer(const SceneRenderer&) = delete;
        SceneRenderer& operator=(const SceneRenderer&) = delete;
        SceneRenderer(SceneRenderer&&) noexcept = default;
        SceneRenderer& operator=(SceneRenderer&&) noexcept = default;

        bool Initialize(IRHI* rhi);
        void Shutdown();

        // Renders the scene into active frame render pass using default scene view matrix.
        void Render(Scene *scene, IRHI* rhi);

        // Renders the scene using a specific camera (SceneCamera or EditorCamera).
        void Render(Scene *scene, const ICamera& camera, IRHI* rhi);

        // Retrieves or creates a Vulkan descriptor set cached by camera memory address.
        VkDescriptorSet GetOrCreateCameraBindingSet(const ICamera* camera);
        void ClearCameraCache();

        // Handles offscreen target resize.
        void OnResize(uint32_t width, uint32_t height, IRHI* rhi);

        [[nodiscard]] Ref<VulkanGraphicsPipeline> GetPipeline(const PipelineType &type) noexcept;
        [[nodiscard]] Batch2D&                    GetBatch2D()                          noexcept { return m_Batch2D; }

    private:

        VulkanShader           m_VertShader;
        VulkanShader           m_FragShader;

        std::map<PipelineType, Ref<VulkanGraphicsPipeline>> m_Pipelines;
        SceneGraph             m_SceneGraph;
        Batch2D                m_Batch2D;

        VkDevice               m_Device          = VK_NULL_HANDLE;
        VkPhysicalDevice       m_PhysicalDevice  = VK_NULL_HANDLE;

        std::unordered_map<std::uintptr_t, CameraCacheEntry> m_CameraCache;

        // std::vector<std::unique_ptr<VulkanTexture>> m_LoadedTextures;
        Scope<VulkanTexture> m_WhiteTexture;

        std::array<VkImageView, PIPELINE_BACKBUFFER_COUNT> m_LastImageViews{};
        bool                   m_Initialized = false;
    };

}
