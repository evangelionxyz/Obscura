#pragma once

#include <Obscura/API.hpp>
#include <Obscura/IRHI.hpp>

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace Obscura
{
    template <typename T>
    class LockFreeTripleBuffer
    {
    public:
        LockFreeTripleBuffer()
            : m_WriteIndex(0), m_ReadIndex(1), m_LatestIndex(2), m_HasNewFrame(false)
        {
        }

        [[nodiscard]] T& GetWriteBuffer() noexcept
        {
            return m_Buffers[m_WriteIndex];
        }

        void PublishWriteBuffer() noexcept
        {
            std::uint32_t prevLatest = m_LatestIndex.exchange(m_WriteIndex, std::memory_order_release);
            m_WriteIndex = prevLatest;
            m_HasNewFrame.store(true, std::memory_order_release);
        }

        bool AcquireLatestReadBuffer(T*& outBuffer) noexcept
        {
            if (!m_HasNewFrame.load(std::memory_order_acquire))
            {
                outBuffer = &m_Buffers[m_ReadIndex];
                return false;
            }

            std::uint32_t newest = m_LatestIndex.exchange(m_ReadIndex, std::memory_order_acq_rel);
            m_ReadIndex = newest;
            m_HasNewFrame.store(false, std::memory_order_release);

            outBuffer = &m_Buffers[m_ReadIndex];
            return true;
        }

        [[nodiscard]] const T& GetCurrentReadBuffer() const noexcept
        {
            return m_Buffers[m_ReadIndex];
        }

        [[nodiscard]] bool HasNewFrame() const noexcept
        {
            return m_HasNewFrame.load(std::memory_order_acquire);
        }

    private:
        std::array<T, 3>           m_Buffers{};
        std::uint32_t              m_WriteIndex{0};
        std::uint32_t              m_ReadIndex{1};
        std::atomic<std::uint32_t> m_LatestIndex{2};
        std::atomic<bool>          m_HasNewFrame{false};
    };

    struct RenderFramePacket
    {
        std::uint32_t frameNumber = 0;
        float deltaTime = 0.0f;
        std::vector<std::function<void(IRHI*)>> commands;
    };

    class OBSCURA_RENDERER_API RenderCommandQueue
    {
    public:
        RenderCommandQueue() = default;
        ~RenderCommandQueue() = default;

        void PushCommand(std::function<void(IRHI*)> command);
        void SubmitPacket(std::uint32_t frameNumber, float deltaTime);
        bool ExecutePendingPacket(IRHI* rhi);

        [[nodiscard]] std::size_t GetPendingCommandCount() const noexcept;

    private:
        LockFreeTripleBuffer<RenderFramePacket> m_TripleBuffer;
        RenderFramePacket                      m_StagingPacket;
    };
}
