#pragma once

#include <Obscura/IRHI.hpp>

#include <QtGui/QImage>
#include <array>
#include <atomic>
#include <cstdint>

namespace ObscuraEditor
{
    struct FrameBufferPayload
    {
        Obscura::GPUTextureHandle gpuHandle{};
        QImage                    image;
        bool                      isGPUInterop = false;
        bool                      isValid      = false;
    };

    class FrameBufferTransport
    {
    public:
        FrameBufferTransport()
            : m_WriteIndex(0), m_ReadIndex(1), m_LatestIndex(2), m_HasNewFrame(false)
        {
        }
        ~FrameBufferTransport() = default;

        bool UpdateFromRHI(Obscura::IRHI* rhi)
        {
            if (!rhi)
            {
                return false;
            }

            FrameBufferPayload& payload = m_Buffers[m_WriteIndex];

            // Check if RHI provides direct GPU Texture Interop
            Obscura::GPUTextureHandle gpuHandle = rhi->GetGPUTextureHandle();
            if (gpuHandle.isGPUInterop && gpuHandle.nativeHandle != nullptr)
            {
                payload.gpuHandle = gpuHandle;
                payload.isGPUInterop = true;
                payload.isValid = true;
                PublishWriteBuffer();
                return true;
            }

            // Fallback: CPU buffer readback
            const void* data = rhi->GetFrameBufferData();
            const std::uint32_t width = rhi->GetFrameBufferWidth();
            const std::uint32_t height = rhi->GetFrameBufferHeight();
            const std::uint32_t stride = rhi->GetFrameBufferStride();

            if (!data || width == 0 || height == 0)
            {
                return false;
            }

            QImage frame(static_cast<const uchar*>(data),
                         static_cast<int>(width),
                         static_cast<int>(height),
                         static_cast<qsizetype>(stride),
                         QImage::Format_RGBA8888);

            payload.image = frame.copy();
            payload.gpuHandle = Obscura::GPUTextureHandle{};
            payload.isGPUInterop = false;
            payload.isValid = true;
            PublishWriteBuffer();
            return true;
        }

        [[nodiscard]] Obscura::GPUTextureHandle AcquireGpuHandle()
        {
            AcquireLatestReadBuffer();
            return m_Buffers[m_ReadIndex].gpuHandle;
        }

        [[nodiscard]] QImage AcquireFrame()
        {
            AcquireLatestReadBuffer();
            return m_Buffers[m_ReadIndex].image;
        }

        [[nodiscard]] bool HasNewFrame() const noexcept
        {
            return m_HasNewFrame.load(std::memory_order_acquire);
        }

        [[nodiscard]] bool IsGPUInterop() const noexcept
        {
            return m_Buffers[m_ReadIndex].isGPUInterop;
        }

    private:
        void PublishWriteBuffer() noexcept
        {
            std::uint32_t prevLatest = m_LatestIndex.exchange(m_WriteIndex, std::memory_order_release);
            m_WriteIndex = prevLatest;
            m_HasNewFrame.store(true, std::memory_order_release);
        }

        void AcquireLatestReadBuffer() noexcept
        {
            if (m_HasNewFrame.load(std::memory_order_acquire))
            {
                std::uint32_t newest = m_LatestIndex.exchange(m_ReadIndex, std::memory_order_acq_rel);
                m_ReadIndex = newest;
                m_HasNewFrame.store(false, std::memory_order_release);
            }
        }

    private:
        std::array<FrameBufferPayload, 3> m_Buffers{};
        std::uint32_t                     m_WriteIndex{0};
        std::uint32_t                     m_ReadIndex{1};
        std::atomic<std::uint32_t>        m_LatestIndex{2};
        std::atomic<bool>                 m_HasNewFrame{false};
    };
}
