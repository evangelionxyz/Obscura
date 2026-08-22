#include <Obscura/API.hpp>
#include <Obscura/IEngine.hpp>
#include <Obscura/IRHI.hpp>
#include <Obscura/Logger.hpp>
#include <Obscura/Module.hpp>
#include <Obscura/Types.hpp>

#include "Scene.hpp"
#include "SceneRenderer.hpp"

#include <filesystem>
#include <memory>

namespace
{

class EngineImpl : public Obscura::IEngine
{
public:
    EngineImpl() = default;
    ~EngineImpl() override { Shutdown(); }

    bool Initialize(const Obscura::EngineInitParams& params) override
    {
        m_Params = params;
        LOG_INFO("[Engine.dll] Initializing '{}' ({}x{})...", m_Params.AppTitle, m_Params.WindowWidth, m_Params.WindowHeight);

        // Dynamically load Renderer RHI module (loose runtime coupling, preventing circular dependencies)
        const std::filesystem::path rhiPath = "Obscura.Renderer.dll";
        if (!m_RhiModule.Load(rhiPath))
        {
            LOG_ERROR("[Engine.dll] Failed to load RHI module from '{}'", rhiPath.string());
            return false;
        }

        auto createRHIFn = m_RhiModule.GetSymbol<Obscura::CreateRHIFn>("CreateRHI");
        m_DestroyRHIFn   = m_RhiModule.GetSymbol<Obscura::DestroyRHIFn>("DestroyRHI");
        if (!createRHIFn)
        {
            LOG_ERROR("[Engine.dll] Failed to find 'CreateRHI' entry point in RHI module.");
            m_RhiModule.Unload();
            return false;
        }

        m_RHI = createRHIFn(Obscura::RHI_ABI_VERSION);
        if (!m_RHI)
        {
            LOG_ERROR("[Engine.dll] Failed to create RHI instance (ABI version mismatch).");
            m_RhiModule.Unload();
            return false;
        }

        if (!m_RHI->Initialize())
        {
            LOG_ERROR("[Engine.dll] Failed to initialize RHI backend!");
            if (m_DestroyRHIFn)
            {
                m_DestroyRHIFn(m_RHI);
            }
            else
            {
                m_RHI->Destroy();
            }
            m_RHI = nullptr;
            m_RhiModule.Unload();
            return false;
        }

        LOG_INFO("[Engine.dll] Attached RHI Backend: {}", m_RHI->GetName());

        // Initialize Engine SceneRenderer
        m_SceneRenderer = std::make_unique<Obscura::SceneRenderer>();
        if (!m_SceneRenderer->Initialize(m_RHI))
        {
            LOG_WARN("[Engine.dll] SceneRenderer failed to initialize — scene draws will be disabled.");
        }
        else
        {
            // Populate default sprite entity
            auto entity = m_Scene.CreateEntity("DefaultSprite");
            auto& sprite = m_Scene.AddComponent<Obscura::Sprite2D>(entity);
            sprite.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            sprite.useTexture = false;
            LOG_INFO("[Engine.dll] Default sprite entity added to scene.");
        }

        LOG_INFO("[Engine.dll] Engine initialized successfully.");
        return true;
    }

    void Shutdown() override
    {
        if (m_SceneRenderer)
        {
            m_SceneRenderer->Shutdown();
            m_SceneRenderer.reset();
        }
        m_Scene.Clear();

        if (m_RHI)
        {
            LOG_INFO("[Engine.dll] Shutting down Engine...");
            m_RHI->Shutdown();
            if (m_DestroyRHIFn)
            {
                m_DestroyRHIFn(m_RHI);
            }
            else
            {
                m_RHI->Destroy();
            }
            m_RHI = nullptr;
            m_RhiModule.Unload();
            LOG_INFO("[Engine.dll] Engine shutdown complete.");
        }
    }

    void Tick(float deltaTime) override
    {
        m_FrameCount++;
        if (m_RHI)
        {
            m_RHI->BeginFrame();

            if (m_SceneRenderer && m_SceneRenderer->IsValid())
            {
                m_SceneRenderer->Render(m_Scene, m_RHI);
            }

            m_RHI->EndFrame();
        }
    }

    const char* GetVersion() const override
    {
        return "0.1.0-prototype";
    }

    Obscura::IRHI* GetRHI() const override
    {
        return m_RHI;
    }

    void Destroy() override
    {
        delete this;
    }

private:
    Obscura::EngineInitParams                  m_Params{};
    Obscura::Module                            m_RhiModule;
    Obscura::IRHI*                             m_RHI          = nullptr;
    Obscura::DestroyRHIFn                      m_DestroyRHIFn = nullptr;
    uint32_t                                   m_FrameCount   = 0;

    Obscura::Scene                             m_Scene;
    std::unique_ptr<Obscura::SceneRenderer>    m_SceneRenderer;
};

} // anonymous namespace

extern "C"
{
    OBSCURA_ENGINE_API Obscura::IEngine* CreateEngine(uint32_t abiVersion)
    {
        if (abiVersion != Obscura::ENGINE_ABI_VERSION)
        {
            LOG_ERROR("[Engine.dll] ABI Version mismatch! Host requested version {}, but DLL is compiled with version {}",
                abiVersion, Obscura::ENGINE_ABI_VERSION);
            return nullptr;
        }
        return new EngineImpl();
    }

    OBSCURA_ENGINE_API void DestroyEngine(Obscura::IEngine* engine)
    {
        if (engine)
        {
            engine->Destroy();
        }
    }
}
