#pragma once

#include <Obscura/API.hpp>
#include <Obscura/IRHI.hpp>
#include <Obscura/Types.hpp>

#include <cstdint>

namespace Obscura
{
    class D3D12RHI : public IRHI
    {
    public:
        D3D12RHI() = default;
        ~D3D12RHI() override = default;

        bool        Initialize() override;
        void        Shutdown() override;
        void        BeginFrame() override;
        void        EndFrame() override;
        void        Destroy() override;
        const char *GetName() const override;

        bool        CreateOffscreenTarget(std::uint32_t width, std::uint32_t height) override;
        void        DestroyOffscreenTarget() override;
        bool        ResizeOffscreenTarget(std::uint32_t width, std::uint32_t height) override;
        const void* GetFrameBufferData() const override;
        std::uint32_t GetFrameBufferStride() const override;
        std::uint32_t GetFrameBufferWidth() const override;
        std::uint32_t GetFrameBufferHeight() const override;
        void        SetRenderMode(RenderMode mode) override;

    private:
        RenderMode    m_RenderMode      = RenderMode::Window;
        std::uint32_t m_OffscreenWidth  = 0;
        std::uint32_t m_OffscreenHeight = 0;
        bool          m_Initialized     = false;
    };
}
