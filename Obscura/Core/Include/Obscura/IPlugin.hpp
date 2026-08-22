#pragma once

#include "Obscura/Types.hpp"
#include "Obscura/IEngine.hpp"

namespace Obscura
{
    struct IPlugin
    {
        virtual ~IPlugin() = default;

        /// Return metadata about this plugin (name, version, author, ABI version).
        virtual PluginInfo  GetInfo() const = 0;

        /// Called once after the plugin is loaded. The engine pointer is valid
        /// for the entire lifetime of the plugin.
        virtual bool        Initialize(IEngine* engine) = 0;

        /// Called once before the plugin is unloaded.
        virtual void        Shutdown() = 0;

        /// Called every frame by the PluginManager.
        virtual void        Tick(float deltaTime) = 0;

        /// Release resources and delete self. Called by DestroyPlugin().
        virtual void        Destroy() = 0;
    };

    // Function pointer typedefs for dynamic loading
    using CreatePluginFn  = IPlugin* (*)();
    using DestroyPluginFn = void (*)(IPlugin* plugin);

}
