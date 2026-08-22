#include "EngineModule.hpp"
#include <Obscura/Logger.hpp>

#include <filesystem>

namespace Obscura
{
    bool EngineModule::Load(const std::filesystem::path &path)
    {
        Shutdown();

        if (!m_Module.Load(path))
            return false;

        auto createFn = m_Module.GetSymbol<Obscura::CreateEngineFn>("CreateEngine");
        if (!createFn)
        {
            LOG_ERROR("[EngineModule] Failed to find 'CreateEngine' symbol in DLL.");
            m_Module.Unload();
            return false;
        }

        m_Engine = createFn(Obscura::ENGINE_ABI_VERSION);
        if (!m_Engine)
        {
            LOG_ERROR("[EngineModule] Failed to create Engine instance (ABI version mismatch).");
            m_Module.Unload();
            return false;
        }

        return true;
    }

    void EngineModule::Shutdown()
    {
        if (m_Engine)
        {
            m_Engine->Shutdown();

            auto destroyFn = m_Module.GetSymbol<Obscura::DestroyEngineFn>("DestroyEngine");
            if (destroyFn)
            {
                destroyFn(m_Engine);
            }
            else
            {
                m_Engine->Destroy();
            }

            m_Engine = nullptr;
        }
        m_Module.Unload();
    }
}
