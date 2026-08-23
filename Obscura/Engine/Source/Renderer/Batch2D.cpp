#include "Batch2D.hpp"
#include "GPUData.hpp"

#include "Scene/Scene.hpp"
#include <Obscura/Logger.hpp>

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
#include <cstring>
#include <algorithm>

namespace Obscura
{
    static constexpr uint32_t s_BatchGrowThresholdPercent   = 90;
    static constexpr uint32_t s_BatchShrinkThresholdPercent = 30;
    static constexpr uint32_t s_BatchShrinkFrameThreshold   = 300;

    bool Batch2D::Initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue)
    {
        if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE)
        {
            LOG_ERROR("[Batch2D] Invalid Vulkan handles passed to Initialize.");
            return false;
        }

        m_Device         = device;
        m_PhysicalDevice = physicalDevice;
        m_CommandPool    = commandPool;
        m_Queue          = queue;

        InitQuadData();

        m_Initialized = true;
        LOG_INFO("[Batch2D] Initialized 2D batch renderer (Initial Capacity: {} quads).", m_QuadBatch.maxCount);
        return true;
    }

    void Batch2D::Shutdown()
    {
        if (!m_Initialized)
        {
            return;
        }

        delete[] m_QuadBatch.vertexBufferBase;
        m_QuadBatch.vertexBufferBase = nullptr;
        m_QuadBatch.vertexBufferPtr  = nullptr;

        for (auto& vb : m_QuadBatch.vertexBuffers)
        {
            vb.Destroy();
        }
        m_QuadBatch.bufferOffsets.fill(0);

        m_QuadBatch.indexBuffer.Destroy();

        m_Initialized = false;
        LOG_INFO("[Batch2D] Batch2D shutdown complete.");
    }

    void Batch2D::InitQuadData()
    {
        m_QuadBatch.minCount          = DefaultMinQuads;
        m_QuadBatch.maxCount          = m_QuadBatch.minCount;
        m_QuadBatch.verticesPerObject = 4;
        m_QuadBatch.indicesPerObject  = 6;
        m_QuadBatch.maxVertices       = m_QuadBatch.maxCount * m_QuadBatch.verticesPerObject;
        m_QuadBatch.maxIndices        = m_QuadBatch.maxCount * m_QuadBatch.indicesPerObject;
        m_QuadBatch.lowUsageFrames    = 0;
        m_QuadBatch.count             = 0;
        m_QuadBatch.indexCount        = 0;

        const size_t vertAllocSize    = static_cast<size_t>(m_QuadBatch.maxVertices) * sizeof(Quad2DVertex);
        const size_t indicesAllocSize = static_cast<size_t>(m_QuadBatch.maxIndices) * sizeof(uint32_t);

        m_QuadBatch.vertexBufferBase  = new Quad2DVertex[m_QuadBatch.maxVertices];
        m_QuadBatch.vertexBufferPtr   = m_QuadBatch.vertexBufferBase;

        // 1. Create persistently mapped dynamic vertex buffer for each backbuffer frame
        for (uint32_t i = 0; i < PIPELINE_BACKBUFFER_COUNT; ++i)
        {
            if (!m_QuadBatch.vertexBuffers[i].CreateDynamic(m_Device, m_PhysicalDevice, vertAllocSize))
            {
                LOG_ERROR("[Batch2D] Failed to create dynamic vertex buffer for frame {}.", i);
            }
            m_QuadBatch.bufferOffsets[i] = 0;
        }

        // 2. Create Static Device-Local Index Buffer with CCW winding
        std::vector<uint32_t> indices(m_QuadBatch.maxIndices);
        uint32_t offset = 0;

        // CCW winding: after Vulkan Y-flip in projection, vertices are Y-down on screen.
        // 0(BL),2(TR),1(BR) and 0(BL),3(TL),2(TR) match VK_FRONT_FACE_COUNTER_CLOCKWISE.
        for (uint32_t i = 0; i < m_QuadBatch.maxIndices; i += 6)
        {
            indices[i + 0] = offset + 0;
            indices[i + 1] = offset + 2;
            indices[i + 2] = offset + 1;

            indices[i + 3] = offset + 0;
            indices[i + 4] = offset + 3;
            indices[i + 5] = offset + 2;

            offset += 4;
        }

        if (!m_QuadBatch.indexBuffer.CreateDynamic(m_Device, m_PhysicalDevice,
                                                   indices.data(), indicesAllocSize,
                                                   m_QuadBatch.maxIndices, VK_INDEX_TYPE_UINT32))
        {
            LOG_ERROR("[Batch2D] Failed to create initial dynamic index buffer ({} bytes).", indicesAllocSize);
        }

        // Standard Quad Unit Local Positions
        m_QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f }; // bottom-left
        m_QuadVertexPositions[1] = {  0.5f, -0.5f, 0.0f, 1.0f }; // bottom-right
        m_QuadVertexPositions[2] = {  0.5f,  0.5f, 0.0f, 1.0f }; // top-right
        m_QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f }; // top-left
    }

    void Batch2D::ResizeQuadBatch(uint32_t newMaxCount)
    {
        if (newMaxCount == 0 || newMaxCount == m_QuadBatch.maxCount)
        {
            return;
        }

        const uint32_t usedVertices = m_QuadBatch.vertexBufferPtr
            ? static_cast<uint32_t>(m_QuadBatch.vertexBufferPtr - m_QuadBatch.vertexBufferBase)
            : 0;

        auto* newBase = new Quad2DVertex[newMaxCount * m_QuadBatch.verticesPerObject];
        if (m_QuadBatch.vertexBufferBase && usedVertices > 0)
        {
            std::copy_n(m_QuadBatch.vertexBufferBase, usedVertices, newBase);
        }

        delete[] m_QuadBatch.vertexBufferBase;
        m_QuadBatch.vertexBufferBase = newBase;
        m_QuadBatch.vertexBufferPtr  = m_QuadBatch.vertexBufferBase + usedVertices;

        m_QuadBatch.maxCount    = newMaxCount;
        m_QuadBatch.maxVertices = m_QuadBatch.maxCount * m_QuadBatch.verticesPerObject;
        m_QuadBatch.maxIndices  = m_QuadBatch.maxCount * m_QuadBatch.indicesPerObject;

        const size_t verticesAllocSize = static_cast<size_t>(m_QuadBatch.maxVertices) * sizeof(Quad2DVertex);
        const size_t indicesAllocSize  = static_cast<size_t>(m_QuadBatch.maxIndices) * sizeof(uint32_t);

        // Recreate host-visible vertex buffers with new size for all frames
        for (uint32_t i = 0; i < PIPELINE_BACKBUFFER_COUNT; ++i)
        {
            m_QuadBatch.vertexBuffers[i].Destroy();
            if (!m_QuadBatch.vertexBuffers[i].CreateDynamic(m_Device, m_PhysicalDevice, verticesAllocSize))
            {
                LOG_ERROR("[Batch2D] Failed to resize dynamic vertex buffer for frame {} ({} bytes)", i, verticesAllocSize);
            }
            m_QuadBatch.bufferOffsets[i] = 0;
        }

        // Recreate dynamic index buffer with new capacity
        m_QuadBatch.indexBuffer.Destroy();

        std::vector<uint32_t> indices(m_QuadBatch.maxIndices);
        uint32_t offset = 0;
        for (uint32_t i = 0; i < m_QuadBatch.maxIndices; i += 6)
        {
            indices[i + 0] = offset + 0;
            indices[i + 1] = offset + 2;
            indices[i + 2] = offset + 1;

            indices[i + 3] = offset + 0;
            indices[i + 4] = offset + 3;
            indices[i + 5] = offset + 2;

            offset += 4;
        }

        if (!m_QuadBatch.indexBuffer.CreateDynamic(m_Device, m_PhysicalDevice,
                                                   indices.data(), indicesAllocSize,
                                                   m_QuadBatch.maxIndices, VK_INDEX_TYPE_UINT32))
        {
            LOG_ERROR("[Batch2D] Failed to resize dynamic index buffer ({} bytes)", indicesAllocSize);
        }

        if (m_QuadBatch.count > m_QuadBatch.maxCount)
            m_QuadBatch.count = m_QuadBatch.maxCount;
        if (m_QuadBatch.indexCount > m_QuadBatch.maxIndices)
            m_QuadBatch.indexCount = m_QuadBatch.maxIndices;

        LOG_INFO("[Batch2D] Resized quad batch buffer capacity to {} quads (Vertex: {} KB, Index: {} KB)",
                 m_QuadBatch.maxCount, verticesAllocSize / 1024, indicesAllocSize / 1024);
    }

    void Batch2D::EnsureQuadBatchCapacity(uint32_t additionalVertices, uint32_t additionalIndices)
    {
        const uint32_t usedVertices = m_QuadBatch.vertexBufferPtr
            ? static_cast<uint32_t>(m_QuadBatch.vertexBufferPtr - m_QuadBatch.vertexBufferBase)
            : 0;

        const uint32_t requiredVertices = usedVertices + additionalVertices;
        const uint32_t requiredIndices  = m_QuadBatch.indexCount + additionalIndices;

        const uint32_t vertexGrowThreshold = (m_QuadBatch.maxVertices * s_BatchGrowThresholdPercent) / 100;
        const uint32_t indexGrowThreshold  = m_QuadBatch.maxIndices > 0
            ? (m_QuadBatch.maxIndices * s_BatchGrowThresholdPercent) / 100
            : 0;

        const bool needGrowByVertex = requiredVertices >= vertexGrowThreshold;
        const bool needGrowByIndex  = m_QuadBatch.indicesPerObject > 0 && requiredIndices >= indexGrowThreshold;

        if (!needGrowByVertex && !needGrowByIndex)
        {
            return;
        }

        uint32_t newMaxCount = m_QuadBatch.maxCount;
        while (true)
        {
            const uint32_t newMaxVertices = newMaxCount * m_QuadBatch.verticesPerObject;
            const uint32_t newMaxIndices  = newMaxCount * m_QuadBatch.indicesPerObject;

            const bool fitVertices = requiredVertices < (newMaxVertices * s_BatchGrowThresholdPercent) / 100;
            const bool fitIndices  = m_QuadBatch.indicesPerObject == 0 || requiredIndices < (newMaxIndices * s_BatchGrowThresholdPercent) / 100;

            if (fitVertices && fitIndices)
            {
                break;
            }

            newMaxCount *= 2;
        }

        ResizeQuadBatch(newMaxCount);
        m_QuadBatch.lowUsageFrames = 0;
    }

    void Batch2D::TryShrinkQuadBatch(uint32_t usedVertices, uint32_t usedIndices)
    {
        if (m_QuadBatch.maxCount <= m_QuadBatch.minCount)
        {
            return;
        }

        const bool lowVertexUsage = usedVertices < (m_QuadBatch.maxVertices * s_BatchShrinkThresholdPercent) / 100;
        const bool lowIndexUsage  = m_QuadBatch.indicesPerObject == 0 || usedIndices < (m_QuadBatch.maxIndices * s_BatchShrinkThresholdPercent) / 100;

        if (lowVertexUsage && lowIndexUsage)
        {
            m_QuadBatch.lowUsageFrames++;
        }
        else
        {
            m_QuadBatch.lowUsageFrames = 0;
            return;
        }

        if (m_QuadBatch.lowUsageFrames < s_BatchShrinkFrameThreshold)
        {
            return;
        }

        const uint32_t targetByVertices = std::max(m_QuadBatch.minCount,
            static_cast<uint32_t>(std::max<uint32_t>(1, usedVertices) * 2 / std::max<uint32_t>(1, m_QuadBatch.verticesPerObject)));

        const uint32_t targetByIndices = m_QuadBatch.indicesPerObject > 0
            ? std::max(m_QuadBatch.minCount, static_cast<uint32_t>(std::max<uint32_t>(1, usedIndices) * 2 / std::max<uint32_t>(1, m_QuadBatch.indicesPerObject)))
            : m_QuadBatch.minCount;

        const uint32_t target = std::max(targetByVertices, targetByIndices);
        const uint32_t newMaxCount = std::max(m_QuadBatch.minCount, std::max(target, m_QuadBatch.maxCount / 2));

        if (newMaxCount < m_QuadBatch.maxCount)
        {
            ResizeQuadBatch(newMaxCount);
        }

        m_QuadBatch.lowUsageFrames = 0;
    }

    void Batch2D::BeginFrame(uint32_t frameIndex)
    {
        m_CurrentFrameIndex = frameIndex % PIPELINE_BACKBUFFER_COUNT;
        m_QuadBatch.bufferOffsets[m_CurrentFrameIndex] = 0;
        m_Stats.Reset();
    }

    void Batch2D::BeginBatch(const glm::mat4& viewProjection, uint32_t frameIndex)
    {
        uint32_t safeFrameIndex = frameIndex % PIPELINE_BACKBUFFER_COUNT;
        if (safeFrameIndex != m_CurrentFrameIndex)
        {
            BeginFrame(safeFrameIndex);
        }

        m_ViewProjection            = viewProjection;
        m_QuadBatch.vertexBufferPtr = m_QuadBatch.vertexBufferBase;
        m_QuadBatch.count           = 0;
        m_QuadBatch.indexCount      = 0;
        m_CurrentTextureSlot        = 0;
        m_CurrentUseTexture         = 0;
    }

    void Batch2D::EndBatch()
    {
        const uint32_t usedVertices = m_QuadBatch.vertexBufferPtr
            ? static_cast<uint32_t>(m_QuadBatch.vertexBufferPtr - m_QuadBatch.vertexBufferBase)
            : 0;

        TryShrinkQuadBatch(usedVertices, m_QuadBatch.indexCount);
    }

    void Batch2D::Flush(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout)
    {
        const uint32_t usedVertices = m_QuadBatch.vertexBufferPtr
            ? static_cast<uint32_t>(m_QuadBatch.vertexBufferPtr - m_QuadBatch.vertexBufferBase)
            : 0;

        if (usedVertices == 0 || m_QuadBatch.indexCount == 0 || !cmd || pipelineLayout == VK_NULL_HANDLE)
        {
            return;
        }

        const size_t usedBytes = static_cast<size_t>(usedVertices) * sizeof(Quad2DVertex);
        const size_t maxBufferBytes = static_cast<size_t>(m_QuadBatch.maxVertices) * sizeof(Quad2DVertex);

        // If the batch would overflow the current frame's buffer, grow the buffer
        if (m_QuadBatch.bufferOffsets[m_CurrentFrameIndex] + usedBytes > maxBufferBytes)
        {
            ResizeQuadBatch(m_QuadBatch.maxCount * 2);
        }

        const size_t currentOffset = m_QuadBatch.bufferOffsets[m_CurrentFrameIndex];

        // 1. Direct memory copy into the persistently mapped dynamic vertex buffer (no staging, no GPU wait!)
        m_QuadBatch.vertexBuffers[m_CurrentFrameIndex].SetData(m_QuadBatch.vertexBufferBase, usedBytes, currentOffset);

        // 2. Bind vertex and index buffers
        VkBuffer     buffers[] = { m_QuadBatch.vertexBuffers[m_CurrentFrameIndex].GetBuffer() };
        VkDeviceSize offsets[] = { currentOffset };

        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
        vkCmdBindIndexBuffer(cmd, m_QuadBatch.indexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

        // 3. Set push constants
        SpritePushConstants pushConstants{};
        pushConstants.model       = m_ViewProjection;
        pushConstants.color       = glm::vec4(1.0f); // Pre-multiplied on CPU
        pushConstants.uvOffset    = glm::vec2(0.0f);
        pushConstants.uvScale     = glm::vec2(1.0f);
        pushConstants.textureSlot = m_CurrentTextureSlot;
        pushConstants.useTexture  = m_CurrentUseTexture;

        vkCmdPushConstants(cmd,
                           pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0,
                           sizeof(SpritePushConstants),
                           &pushConstants);

        // 4. Issue draw call
        vkCmdDrawIndexed(cmd, m_QuadBatch.indexCount, 1, 0, 0, 0);

        // 5. Update stats and advance sub-allocation offset
        m_Stats.drawCalls++;
        m_Stats.quadCount += m_QuadBatch.count;
        m_QuadBatch.bufferOffsets[m_CurrentFrameIndex] += usedBytes;

        // 6. Reset batch CPU pointer for subsequent draws
        m_QuadBatch.vertexBufferPtr = m_QuadBatch.vertexBufferBase;
        m_QuadBatch.count           = 0;
        m_QuadBatch.indexCount      = 0;
    }

    void Batch2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color,
                           uint32_t textureSlot, bool useTexture,
                           const glm::vec2& uvOffset, const glm::vec2& uvScale)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));
        DrawQuad(transform, color, textureSlot, useTexture, uvOffset, uvScale);
    }

    void Batch2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color,
                           uint32_t textureSlot, bool useTexture,
                           const glm::vec2& uvOffset, const glm::vec2& uvScale)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0.0f, 0.0f, 1.0f))
            * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

        DrawQuad(transform, color, textureSlot, useTexture, uvOffset, uvScale);
    }

    void Batch2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color,
                           uint32_t textureSlot, bool useTexture,
                           const glm::vec2& uvOffset, const glm::vec2& uvScale)
    {
        EnsureQuadBatchCapacity(4, 6);

        m_CurrentTextureSlot = textureSlot;
        m_CurrentUseTexture  = useTexture ? 1u : 0u;

        constexpr glm::vec2 uvs[4] = {
            { 0.0f, 0.0f },
            { 1.0f, 0.0f },
            { 1.0f, 1.0f },
            { 0.0f, 1.0f }
        };

        for (uint32_t i = 0; i < 4; ++i)
        {
            m_QuadBatch.vertexBufferPtr->position = glm::vec3(transform * m_QuadVertexPositions[i]);
            m_QuadBatch.vertexBufferPtr->uv       = uvs[i] * uvScale + uvOffset;
            m_QuadBatch.vertexBufferPtr->color    = color;
            m_QuadBatch.vertexBufferPtr++;
        }

        m_QuadBatch.indexCount += 6;
        m_QuadBatch.count++;
    }

    void Batch2D::DrawRect(const glm::mat4& transform, const glm::vec4& color,
                           uint32_t textureSlot, bool useTexture)
    {
        DrawQuad(transform, color, textureSlot, useTexture);
    }

    void Batch2D::DrawBox(const glm::mat4& transform, const glm::vec4& color)
    {
        DrawQuad(transform, color, 0, false);
    }
}
