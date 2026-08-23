#pragma once

#include <Obscura/API.hpp>
#include "VertexData.hpp"
#include <Vulkan/VulkanBuffers.hpp>
#include <Vulkan/VulkanGraphicsPipeline.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <vector>
#include <cstdint>

namespace Obscura
{
    template<typename VertexType>
    struct BatchData
    {
        uint32_t minCount = 256;
        uint32_t maxCount = minCount;
        uint32_t verticesPerObject = 4;
        uint32_t indicesPerObject = 6;
        uint32_t maxVertices = maxCount * verticesPerObject;
        uint32_t maxIndices = maxCount * indicesPerObject;
        uint32_t lowUsageFrames = 0;
        uint32_t indexCount = 0;
        uint32_t count = 0;

        VertexType* vertexBufferBase = nullptr;
        VertexType* vertexBufferPtr  = nullptr;
        std::array<VulkanVertexBuffer, PIPELINE_BACKBUFFER_COUNT> vertexBuffers;
        std::array<size_t, PIPELINE_BACKBUFFER_COUNT> bufferOffsets = { 0 };
        VulkanIndexBuffer indexBuffer;

        ~BatchData()
        {
            delete[] vertexBufferBase;
            vertexBufferBase = nullptr;
            vertexBufferPtr = nullptr;
        }
    };

    class OBSCURA_ENGINE_API Batch2D
    {
    public:
        static constexpr uint32_t DefaultMinQuads = 256;

        struct Stats
        {
            uint32_t drawCalls = 0;
            uint32_t quadCount = 0;
            void Reset() { drawCalls = 0; quadCount = 0; }
        };

    public:
        Batch2D() = default;
        ~Batch2D() { Shutdown(); }

        bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue);
        void Shutdown();

        // Frame and batch demarcation
        void BeginFrame(uint32_t frameIndex);
        void BeginBatch(const glm::mat4& viewProjection = glm::mat4(1.0f), uint32_t frameIndex = 0);
        void EndBatch();
        void Flush(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);

        // Quad drawing methods
        void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color = glm::vec4(1.0f),
                      uint32_t textureSlot = 0, bool useTexture = false,
                      const glm::vec2& uvOffset = glm::vec2(0.0f), const glm::vec2& uvScale = glm::vec2(1.0f));

        void DrawQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color = glm::vec4(1.0f),
                      uint32_t textureSlot = 0, bool useTexture = false,
                      const glm::vec2& uvOffset = glm::vec2(0.0f), const glm::vec2& uvScale = glm::vec2(1.0f));

        void DrawQuad(const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f),
                      uint32_t textureSlot = 0, bool useTexture = false,
                      const glm::vec2& uvOffset = glm::vec2(0.0f), const glm::vec2& uvScale = glm::vec2(1.0f));

        // Convenience drawing helpers (from Ignite reference)
        void DrawRect(const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f),
                      uint32_t textureSlot = 0, bool useTexture = false);
        void DrawBox(const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));

        [[nodiscard]] bool IsInitialized() const noexcept { return m_Initialized; }
        [[nodiscard]] const Stats& GetStats() const noexcept { return m_Stats; }
        void ResetStats() noexcept { m_Stats.Reset(); }
        [[nodiscard]] uint32_t GetQuadCount() const noexcept { return m_QuadBatch.count; }
        [[nodiscard]] uint32_t GetCurrentTextureSlot() const noexcept { return m_CurrentTextureSlot; }
        [[nodiscard]] uint32_t GetCurrentUseTexture() const noexcept { return m_CurrentUseTexture; }
        [[nodiscard]] uint32_t GetMaxQuads() const noexcept { return m_QuadBatch.maxCount; }
        [[nodiscard]] uint32_t GetCurrentFrameIndex() const noexcept { return m_CurrentFrameIndex; }

    private:
        void InitQuadData();
        void ResizeQuadBatch(uint32_t newMaxCount);
        void EnsureQuadBatchCapacity(uint32_t additionalVertices, uint32_t additionalIndices);
        void TryShrinkQuadBatch(uint32_t usedVertices, uint32_t usedIndices);

    private:
        VkDevice         m_Device         = VK_NULL_HANDLE;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkCommandPool    m_CommandPool    = VK_NULL_HANDLE;
        VkQueue          m_Queue          = VK_NULL_HANDLE;

        BatchData<Quad2DVertex> m_QuadBatch;

        glm::mat4        m_ViewProjection     = glm::mat4(1.0f);
        uint32_t         m_CurrentFrameIndex  = 0;
        uint32_t         m_CurrentTextureSlot = 0;
        uint32_t         m_CurrentUseTexture  = 0;
        Stats            m_Stats;
        bool             m_Initialized        = false;

        glm::vec4        m_QuadVertexPositions[4];
    };
}
