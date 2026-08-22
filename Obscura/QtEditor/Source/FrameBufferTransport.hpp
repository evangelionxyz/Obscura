#pragma once

#include <Obscura/IRHI.hpp>

#include <QtGui/QImage>
#include <mutex>
#include <cstdint>

namespace ObscuraEditor
{
    class FrameBufferTransport
    {
    public:
        FrameBufferTransport() = default;
        ~FrameBufferTransport() = default;

        bool UpdateFromRHI(Obscura::IRHI* rhi)
        {
            if (!rhi)
            {
                return false;
            }

            // Check if RHI provides direct GPU Texture Interop
            Obscura::GPUTextureHandle gpuHandle = rhi->GetGPUTextureHandle();
            if (gpuHandle.isGPUInterop && gpuHandle.nativeHandle != nullptr)
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                m_GpuHandle = gpuHandle;
                m_HasNewFrame = true;
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

            std::lock_guard<std::mutex> lock(m_Mutex);
            m_CurrentImage = frame.copy();
            m_GpuHandle.isGPUInterop = false;
            m_HasNewFrame = true;
            return true;
        }

        [[nodiscard]] Obscura::GPUTextureHandle AcquireGpuHandle()
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_HasNewFrame = false;
            return m_GpuHandle;
        }

        [[nodiscard]] QImage AcquireFrame()
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_HasNewFrame = false;
            return m_CurrentImage;
        }

        [[nodiscard]] bool HasNewFrame() const
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return m_HasNewFrame;
        }

        [[nodiscard]] bool IsGPUInterop() const
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return m_GpuHandle.isGPUInterop;
        }

    private:
        mutable std::mutex       m_Mutex;
        Obscura::GPUTextureHandle m_GpuHandle{};
        QImage                   m_CurrentImage;
        bool                     m_HasNewFrame = false;
    };
}
