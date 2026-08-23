#pragma once

#include <cstdint>
#include <memory>

namespace Obscura
{
    // ABI version constants — bump these when interface layouts change
    constexpr std::uint32_t ENGINE_ABI_VERSION = 1;
    constexpr std::uint32_t RHI_ABI_VERSION    = 2;
    constexpr std::uint32_t PLUGIN_ABI_VERSION = 1;

    enum class RenderMode : std::uint8_t
    {
        Window = 0,
        Offscreen
    };

    // Engine initialization parameters
    struct EngineInitParams
    {
        std::uint32_t WindowWidth  = 1280;
        std::uint32_t WindowHeight = 720;
        const char*   AppTitle     = "Obscura App";
    };

    // Plugin metadata returned by IPlugin::GetInfo()
    struct PluginInfo
    {
        const char*   Name       = "Unknown";
        const char*   Version    = "0.0.0";
        const char*   Author     = "Unknown";
        std::uint32_t ABIVersion = PLUGIN_ABI_VERSION;
    };

    // Description struct for serializing entity ECS data across module boundaries
    struct EntityDesc
    {
        std::uint64_t uuid         = 0;
        char          name[128]    = { 0 };
        std::uint64_t parentUuid   = 0;
        bool          hasTransform = false;
        float         position[3]  = { 0.0f, 0.0f, 0.0f };
        float         rotation[3]  = { 0.0f, 0.0f, 0.0f };
        float         scale[3]     = { 1.0f, 1.0f, 1.0f };
        bool          hasSprite2D  = false;
        float         spriteColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        std::uint32_t textureSlot  = 0;
        bool          useTexture   = false;
        float         uvOffset[2]  = { 0.0f, 0.0f };
        float         uvScale[2]   = { 1.0f, 1.0f };
        bool          spriteVisible = true;
    };


    // ---------------------------------------
    // Smart Pointer types
    // ---------------------------------------
    template<typename T>
    using Ref = std::shared_ptr<T>;

    template<typename T>
    using WeakRef = std::weak_ptr<T>;

    template<typename T>
    using Scope = std::unique_ptr<T>;

    template<typename T, typename... Args>
    static Ref<T> CreateRef(Args &&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    static WeakRef<T> CreateWeakRef(const Ref<T> &val)
    {
        return std::weak_ptr<T>(val);
    }

    template<typename T, typename... Args>
    static Scope<T> CreateScope(Args &&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
}
