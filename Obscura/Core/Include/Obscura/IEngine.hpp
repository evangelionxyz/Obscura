#pragma once

#include "Obscura/Types.hpp"
#include "Obscura/IRHI.hpp"

#include <cstdint>

namespace Obscura
{
    struct IEngine
    {
        virtual ~IEngine() = default;

        virtual bool        Initialize(const EngineInitParams& params) = 0;
        virtual void        Shutdown()   = 0;
        virtual void        Tick(float deltaTime) = 0;
        virtual const char* GetVersion() const = 0;
        virtual IRHI*       GetRHI() const = 0;
        virtual void        Destroy()    = 0;
    };

    // Function pointer typedefs for dynamic loading
    using CreateEngineFn  = IEngine* (*)(std::uint32_t abiVersion);
    using DestroyEngineFn = void (*)(IEngine* engine);
    using CreateRHIFn     = IRHI* (*)(std::uint32_t abiVersion);
    using DestroyRHIFn    = void (*)(IRHI* rhi);
}
