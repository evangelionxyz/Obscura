#pragma once

#include "Obscura/IPlugin.hpp"
#include "Obscura/IEngine.hpp"
#include "Obscura/Module.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Obscura
{

    class PluginManager
    {
    public:
        PluginManager()  = default;
        ~PluginManager();

        // Non-copyable, non-movable (owns plugin lifetimes)
        PluginManager(const PluginManager&) = delete;
        PluginManager& operator=(const PluginManager&) = delete;

        /// Scan a directory for plugin DLLs (.dll on Windows, .so on Linux)
        /// and load each one. Plugins that fail to load are skipped with a warning.
        void DiscoverPlugins(const std::filesystem::path& pluginDir);

        /// Load a single plugin DLL by path.
        /// Returns true if the plugin was loaded and created successfully.
        bool LoadPlugin(const std::filesystem::path& path);

        /// Unload a plugin by name. The plugin is shut down and destroyed.
        void UnloadPlugin(const std::string& name);

        /// Initialize all loaded plugins, passing the engine pointer.
        void InitializeAll(IEngine* engine);

        /// Tick all initialized plugins.
        void TickAll(float deltaTime);

        /// Shutdown and unload all plugins in reverse load order.
        void ShutdownAll();

        /// Get the number of loaded plugins.
        [[nodiscard]] std::size_t GetPluginCount() const noexcept;

        /// Get info for all loaded plugins.
        [[nodiscard]] std::vector<PluginInfo> GetLoadedPluginInfos() const;

    private:
        struct LoadedPlugin
        {
            Module          DllModule;
            IPlugin*        Instance    = nullptr;
            DestroyPluginFn DestroyFn   = nullptr;
            bool            Initialized = false;
        };

        std::vector<LoadedPlugin> m_Plugins;
    };

}
