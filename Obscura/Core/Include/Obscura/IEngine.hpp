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

        // Viewport & Input controls
        virtual void HandleViewportResize(std::uint32_t width, std::uint32_t height) = 0;
        virtual void HandleMouseMove(float x, float y, bool rightMouseDown, bool middleMouseDown, bool leftMouseDown) = 0;
        virtual void HandleMouseButton(int button, bool pressed, float x, float y) = 0;
        virtual void HandleKey(int key, bool pressed) = 0;

        // Scene / Entity inspection & manipulation
        virtual std::uint32_t GetEntityCount() const = 0;
        virtual bool          GetEntityDescByIndex(std::uint32_t index, EntityDesc* outDesc) const = 0;
        virtual bool          GetEntityDescByUUID(std::uint64_t uuid, EntityDesc* outDesc) const = 0;
        virtual std::uint64_t CreateEntity(const char* name = "Entity", std::uint64_t parentUuid = 0) = 0;
        virtual bool          DestroyEntity(std::uint64_t uuid) = 0;
        virtual bool          SetEntityName(std::uint64_t uuid, const char* name) = 0;
        virtual bool          SetEntityTransform(std::uint64_t uuid, const float position[3], const float rotation[3], const float scale[3]) = 0;
        virtual bool          SetEntitySprite2D(std::uint64_t uuid, const float color[4], std::uint32_t textureSlot, bool useTexture, const float uvOffset[2], const float uvScale[2], bool visible) = 0;
    };


    // Function pointer typedefs for dynamic loading
    using CreateEngineFn  = IEngine* (*)(std::uint32_t abiVersion);
    using DestroyEngineFn = void (*)(IEngine* engine);
    using CreateRHIFn     = IRHI* (*)(std::uint32_t abiVersion);
    using DestroyRHIFn    = void (*)(IRHI* rhi);
}
