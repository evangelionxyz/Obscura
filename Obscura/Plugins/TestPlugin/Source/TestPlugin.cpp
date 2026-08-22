#include <Obscura/API.hpp>
#include <Obscura/IPlugin.hpp>
#include <Obscura/IEngine.hpp>
#include <Obscura/Logger.hpp>
#include <Obscura/Types.hpp>

class TestPlugin : public Obscura::IPlugin
{
public:
    Obscura::PluginInfo GetInfo() const override
    {
        return Obscura::PluginInfo{
            .Name       = "TestPlugin",
            .Version    = "1.0.0",
            .Author     = "Obscura Team",
            .ABIVersion = Obscura::PLUGIN_ABI_VERSION,
        };
    }

    bool Initialize(Obscura::IEngine* engine) override
    {
        m_Engine = engine;
        LOG_INFO("  [TestPlugin] Initialized! Engine version: {}", m_Engine->GetVersion());
        return true;
    }

    void Shutdown() override
    {
        LOG_INFO("  [TestPlugin] Shutting down.");
        m_Engine = nullptr;
    }

    void Tick(float deltaTime) override
    {
        m_TickCount++;
        if (m_TickCount <= 3) // Only log first 3 ticks to avoid spam
        {
            LOG_INFO("  [TestPlugin] Tick #{} (dt: {:.4f}s)", m_TickCount, deltaTime);
        }
    }

    void Destroy() override
    {
        delete this;
    }

private:
    Obscura::IEngine* m_Engine = nullptr;
    uint32_t m_TickCount = 0;
};

extern "C"
{
    OBSCURA_PLUGIN_API Obscura::IPlugin* CreatePlugin()
    {
        return new TestPlugin();
    }

    OBSCURA_PLUGIN_API void DestroyPlugin(Obscura::IPlugin* plugin)
    {
        if (plugin)
        {
            plugin->Destroy();
        }
    }
}
