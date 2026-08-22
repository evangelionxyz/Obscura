#include "D3D12RHI.hpp"
#include <Obscura/Logger.hpp>

namespace Obscura
{
    bool D3D12RHI::Initialize()
    {
        LOG_INFO("  [D3D12RHI] D3D12 RHI Backend stub initialized.");
        m_Initialized = true;
        return true;
    }

    void D3D12RHI::Shutdown()
    {
        m_Initialized = false;
        LOG_INFO("  [D3D12RHI] D3D12 RHI Backend shutdown.");
    }

    void D3D12RHI::BeginFrame()
    {
    }

    void D3D12RHI::EndFrame()
    {
    }

    const char* D3D12RHI::GetName() const
    {
        return "DirectX 12 (RHI Subsystem Stub)";
    }

    void D3D12RHI::Destroy()
    {
        delete this;
    }

    bool D3D12RHI::CreateOffscreenTarget(std::uint32_t width, std::uint32_t height)
    {
        m_OffscreenWidth = width;
        m_OffscreenHeight = height;
        return true;
    }

    void D3D12RHI::DestroyOffscreenTarget()
    {
        m_OffscreenWidth = 0;
        m_OffscreenHeight = 0;
    }

    bool D3D12RHI::ResizeOffscreenTarget(std::uint32_t width, std::uint32_t height)
    {
        m_OffscreenWidth = width;
        m_OffscreenHeight = height;
        return true;
    }

    const void* D3D12RHI::GetFrameBufferData() const
    {
        return nullptr;
    }

    std::uint32_t D3D12RHI::GetFrameBufferStride() const
    {
        return 0;
    }

    std::uint32_t D3D12RHI::GetFrameBufferWidth() const
    {
        return m_OffscreenWidth;
    }

    std::uint32_t D3D12RHI::GetFrameBufferHeight() const
    {
        return m_OffscreenHeight;
    }

    void D3D12RHI::SetRenderMode(RenderMode mode)
    {
        m_RenderMode = mode;
    }
}
