#include "Obscura/PluginManager.hpp"
#include "Obscura/Logger.hpp"
#include "Obscura/Types.hpp"

#include <algorithm>

namespace Obscura
{
    PluginManager::~PluginManager()
    {
        ShutdownAll();
    }

    void PluginManager::DiscoverPlugins(const std::filesystem::path& pluginDir)
    {
        if (!std::filesystem::exists(pluginDir) || !std::filesystem::is_directory(pluginDir))
        {
            LOG_WARN("[PluginManager] Plugin directory '{}' does not exist, skipping discovery.",
                pluginDir.string());
            return;
        }

        LOG_INFO("[PluginManager] Scanning for plugins in '{}'...", pluginDir.string());

        for (const auto& entry : std::filesystem::directory_iterator(pluginDir))
        {
            if (!entry.is_regular_file())
                continue;

            const auto& filePath = entry.path();
            auto ext = filePath.extension().string();

#if defined(_WIN32)
            if (ext != ".dll")
                continue;
#else
            if (ext != ".so")
                continue;
#endif
            LOG_DEBUG("[PluginManager] Found plugin candidate: {}", filePath.filename().string());
            LoadPlugin(filePath);
        }

        LOG_INFO("[PluginManager] Discovery complete. {} plugin(s) loaded.", m_Plugins.size());
    }

    bool PluginManager::LoadPlugin(const std::filesystem::path& path)
    {
        LoadedPlugin plugin;

        if (!plugin.DllModule.Load(path))
        {
            LOG_ERROR("[PluginManager] Failed to load DLL: {}", path.string());
            return false;
        }

        auto createFn = plugin.DllModule.GetSymbol<CreatePluginFn>("CreatePlugin");
        if (!createFn)
        {
            LOG_ERROR("[PluginManager] '{}' does not export 'CreatePlugin'. Skipping.",
                path.filename().string());
            return false;
        }

        plugin.DestroyFn = plugin.DllModule.GetSymbol<DestroyPluginFn>("DestroyPlugin");
        if (!plugin.DestroyFn)
        {
            LOG_ERROR("[PluginManager] '{}' does not export 'DestroyPlugin'. Skipping.",
                path.filename().string());
            return false;
        }

        plugin.Instance = createFn();
        if (!plugin.Instance)
        {
            LOG_ERROR("[PluginManager] CreatePlugin() returned nullptr for '{}'.",
                path.filename().string());
            return false;
        }

        // Verify ABI version
        auto info = plugin.Instance->GetInfo();
        if (info.ABIVersion != PLUGIN_ABI_VERSION)
        {
            LOG_ERROR(
                "[PluginManager] ABI mismatch for '{}': plugin={}, expected={}. Skipping.",
                info.Name, info.ABIVersion, PLUGIN_ABI_VERSION);
            plugin.DestroyFn(plugin.Instance);
            return false;
        }

        LOG_INFO("[PluginManager] Loaded plugin: '{}' v{} by {} (ABI v{})",
            info.Name, info.Version, info.Author, info.ABIVersion);

        m_Plugins.push_back(std::move(plugin));
        return true;
    }

    void PluginManager::UnloadPlugin(const std::string& name)
    {
        auto it = std::ranges::find_if(m_Plugins, [&](const LoadedPlugin& p)
        {
            return p.Instance && std::string(p.Instance->GetInfo().Name) == name;
        });

        if (it == m_Plugins.end())
        {
            LOG_ERROR("[PluginManager] Plugin '{}' not found.", name);
            return;
        }

        auto& plugin = *it;
        if (plugin.Initialized)
        {
            plugin.Instance->Shutdown();
        }
        if (plugin.DestroyFn)
        {
            plugin.DestroyFn(plugin.Instance);
        }
        plugin.Instance = nullptr;

        LOG_INFO("[PluginManager] Unloaded plugin: '{}'", name);
        m_Plugins.erase(it);
    }

    void PluginManager::InitializeAll(IEngine* engine)
    {
        for (auto& plugin : m_Plugins)
        {
            if (!plugin.Instance || plugin.Initialized)
                continue;

            auto info = plugin.Instance->GetInfo();

            if (plugin.Instance->Initialize(engine))
            {
                plugin.Initialized = true;
                LOG_INFO("[PluginManager] Initialized plugin: '{}'", info.Name);
            }
            else
            {
                LOG_ERROR("[PluginManager] Failed to initialize plugin: '{}'", info.Name);
            }
        }
    }

    void PluginManager::TickAll(float deltaTime)
    {
        for (auto& plugin : m_Plugins)
        {
            if (plugin.Instance && plugin.Initialized)
            {
                plugin.Instance->Tick(deltaTime);
            }
        }
    }

    void PluginManager::ShutdownAll()
    {
        // Shutdown in reverse order of loading
        for (auto it = m_Plugins.rbegin(); it != m_Plugins.rend(); ++it)
        {
            auto& plugin = *it;
            if (!plugin.Instance)
                continue;

            auto info = plugin.Instance->GetInfo();

            if (plugin.Initialized)
            {
                plugin.Instance->Shutdown();
                plugin.Initialized = false;
                LOG_INFO("[PluginManager] Shut down plugin: '{}'", info.Name);
            }

            if (plugin.DestroyFn)
            {
                plugin.DestroyFn(plugin.Instance);
            }
            plugin.Instance = nullptr;
        }

        m_Plugins.clear();
        LOG_INFO("[PluginManager] All plugins unloaded.");
    }

    std::size_t PluginManager::GetPluginCount() const noexcept
    {
        return m_Plugins.size();
    }

    std::vector<PluginInfo> PluginManager::GetLoadedPluginInfos() const
    {
        std::vector<PluginInfo> infos;
        infos.reserve(m_Plugins.size());
        for (const auto& plugin : m_Plugins)
        {
            if (plugin.Instance)
            {
                infos.push_back(plugin.Instance->GetInfo());
            }
        }
        return infos;
    }
}
