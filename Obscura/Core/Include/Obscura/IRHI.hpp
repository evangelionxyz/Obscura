#pragma once

#include "Obscura/Types.hpp"
#include <cstdint>

namespace Obscura
{
    struct VulkanDeviceObjects
    {
        void*         instance         = nullptr;
        void*         physicalDevice   = nullptr;
        void*         device           = nullptr;
        void*         graphicsQueue    = nullptr;
        std::uint32_t queueFamilyIndex = 0;
        std::uint32_t queueIndex       = 0;
    };

    struct GPUTextureHandle
    {
        void*         nativeHandle   = nullptr;
        int           layoutOrState  = 0;
        std::uint32_t width          = 0;
        std::uint32_t height         = 0;
        int           format         = 0;
        bool          isGPUInterop   = false;
    };

    struct IRHI
    {
        virtual ~IRHI() = default;

        virtual bool        Initialize() = 0;
        virtual void        Shutdown()   = 0;
        virtual void        BeginFrame() = 0;
        virtual void        EndFrame()   = 0;
        virtual const char* GetName() const = 0;
        virtual void        Destroy()    = 0;

        virtual bool        CreateOffscreenTarget(std::uint32_t width, std::uint32_t height) = 0;
        virtual void        DestroyOffscreenTarget() = 0;
        virtual bool        ResizeOffscreenTarget(std::uint32_t width, std::uint32_t height) = 0;
        virtual const void* GetFrameBufferData() const = 0;
        virtual std::uint32_t GetFrameBufferStride() const = 0;
        virtual std::uint32_t GetFrameBufferWidth() const = 0;
        virtual std::uint32_t GetFrameBufferHeight() const = 0;
        virtual void        SetRenderMode(RenderMode mode) = 0;

        virtual GPUTextureHandle GetGPUTextureHandle() const { return {}; }
        virtual VulkanDeviceObjects GetVulkanDeviceObjects() const { return {}; }
    };

}

