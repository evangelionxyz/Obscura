#include "EngineModule.hpp"

#include <Obscura/Types.hpp>
#include <Obscura/Logger.hpp>
#include <Obscura/PluginManager.hpp>
#include <Obscura/VFS.hpp>

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <array>

int main([[maybe_unused]] const int argc, [[maybe_unused]] char** argv)
{
    Obscura::Logger::Init();

    LOG_INFO("=== Obscura Engine Host Application Starting ===");

    auto exePath = Obscura::VFS::GetModuleDirectory();
    auto engineDllPath = exePath / "Obscura.Engine.dll";

    LOG_INFO("[Host] Dynamic Engine DLL Path: {}", engineDllPath.string());

    // --- Load Engine DLL ---
    Obscura::EngineModule engine;
    if (!engine.Load(engineDllPath))
    {
        LOG_ERROR("[Host] ERROR: Failed to load Engine module from '{}'", engineDllPath.string());
        Obscura::Logger::Shutdown();
        return 1;
    }

    LOG_INFO("[Host] Engine DLL successfully loaded into host process.");
    LOG_INFO("[Host] Engine Version: {}", engine->GetVersion());

    Obscura::EngineInitParams initParams{
        .WindowWidth = 1920,
        .WindowHeight = 1080,
        .AppTitle = "My Next-Gen Engine Prototype"
    };

    if (!engine->Initialize(initParams))
    {
        LOG_ERROR("[Host] ERROR: Failed to initialize Engine.");
        Obscura::Logger::Shutdown();
        return 1;
    }

    if (auto* rhi = engine->GetRHI())
    {
        LOG_INFO("[Host] Active RHI Subsystem: {}", rhi->GetName());
    }

    // --- Discover and initialize plugins ---
    Obscura::PluginManager pluginManager;
    auto pluginDir = exePath / "Plugins";
    pluginManager.DiscoverPlugins(pluginDir);
    pluginManager.InitializeAll(engine.Get());

    LOG_INFO("--- Starting Simulation Loop (5 frames) ---");
    constexpr float deltaTime = 0.01667f; // ~60 FPS
    for (int frame = 1; frame <= 5; ++frame)
    {
        engine->Tick(deltaTime);
        pluginManager.TickAll(deltaTime);
    }
    LOG_INFO("--- Simulation Loop Finished ---");

    // --- Shutdown ---
    LOG_INFO("[Host] Initiating clean shutdown...");
    pluginManager.ShutdownAll();
    engine.Shutdown();

    LOG_INFO("=== Obscura Engine Host Application Exited Cleanly ===");

    Obscura::Logger::Shutdown();

    return 0;
}
