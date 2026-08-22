#include <Obscura/API.hpp>
#include <Obscura/IEngine.hpp>
#include <Obscura/IRHI.hpp>
#include <Obscura/Logger.hpp>
#include <Obscura/Module.hpp>
#include <Obscura/Types.hpp>

#include <Obscura/WorkerManager.hpp>
#include "Assets/AssetManager.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneRenderer.hpp"

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

        // Initialize core concurrency and asset subsystem
        Obscura::WorkerManager::Get().Initialize();
        Obscura::AssetManager::Get().Initialize();

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

        Obscura::AssetManager::Get().Shutdown();
        Obscura::WorkerManager::Get().Shutdown();
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

    std::uint32_t GetEntityCount() const override
    {
        return static_cast<std::uint32_t>(m_Scene.GetRegistry().view<Obscura::IDComponent>().size());
    }

    bool GetEntityDescByIndex(std::uint32_t index, Obscura::EntityDesc* outDesc) const override
    {
        if (!outDesc) return false;
        auto view = m_Scene.GetRegistry().view<Obscura::IDComponent>();
        std::uint32_t currentIndex = 0;
        for (auto entity : view)
        {
            if (currentIndex == index)
            {
                PopulateEntityDesc(entity, outDesc);
                return true;
            }
            currentIndex++;
        }
        return false;
    }

    bool GetEntityDescByUUID(std::uint64_t uuid, Obscura::EntityDesc* outDesc) const override
    {
        if (!outDesc) return false;
        auto entity = m_Scene.FindEntityByUUID(uuid);
        if (entity == entt::null) return false;
        PopulateEntityDesc(entity, outDesc);
        return true;
    }

    std::uint64_t CreateEntity(const char* name, std::uint64_t parentUuid) override
    {
        std::string entityName = (name && name[0] != '\0') ? name : "Entity";
        auto entity = m_Scene.CreateEntityWithUUID(0, entityName, parentUuid);
        auto& sprite = m_Scene.AddComponent<Obscura::Sprite2D>(entity);
        sprite.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        sprite.useTexture = false;

        return m_Scene.GetComponent<Obscura::IDComponent>(entity).uuid;
    }

    bool DestroyEntity(std::uint64_t uuid) override
    {
        auto entity = m_Scene.FindEntityByUUID(uuid);
        if (entity == entt::null) return false;
        m_Scene.DestroyEntity(entity);
        return true;
    }

    bool SetEntityName(std::uint64_t uuid, const char* name) override
    {
        if (!name) return false;
        auto entity = m_Scene.FindEntityByUUID(uuid);
        if (entity == entt::null) return false;
        if (m_Scene.HasComponent<Obscura::IDComponent>(entity))
        {
            m_Scene.GetComponent<Obscura::IDComponent>(entity).name = name;
        }
        if (m_Scene.HasComponent<Obscura::TagComponent>(entity))
        {
            m_Scene.GetComponent<Obscura::TagComponent>(entity).tag = name;
        }
        return true;
    }

    bool SetEntityTransform(std::uint64_t uuid, const float position[3], const float rotation[3], const float scale[3]) override
    {
        auto entity = m_Scene.FindEntityByUUID(uuid);
        if (entity == entt::null) return false;
        if (!m_Scene.HasComponent<Obscura::Transform>(entity))
        {
            m_Scene.AddComponent<Obscura::Transform>(entity);
        }
        auto& transform = m_Scene.GetComponent<Obscura::Transform>(entity);
        if (position) transform.position = { position[0], position[1], position[2] };
        if (rotation) transform.rotation = { rotation[0], rotation[1], rotation[2] };
        if (scale)    transform.scale    = { scale[0], scale[1], scale[2] };
        return true;
    }

    bool SetEntitySprite2D(std::uint64_t uuid, const float color[4], std::uint32_t textureSlot, bool useTexture, const float uvOffset[2], const float uvScale[2], bool visible) override
    {
        auto entity = m_Scene.FindEntityByUUID(uuid);
        if (entity == entt::null) return false;
        if (!m_Scene.HasComponent<Obscura::Sprite2D>(entity))
        {
            m_Scene.AddComponent<Obscura::Sprite2D>(entity);
        }
        auto& sprite = m_Scene.GetComponent<Obscura::Sprite2D>(entity);
        if (color) sprite.color = { color[0], color[1], color[2], color[3] };
        sprite.textureSlot = textureSlot;
        sprite.useTexture  = useTexture;
        if (uvOffset) sprite.uvOffset = { uvOffset[0], uvOffset[1] };
        if (uvScale)  sprite.uvScale  = { uvScale[0], uvScale[1] };
        sprite.visible     = visible;
        return true;
    }

private:
    void PopulateEntityDesc(entt::entity entity, Obscura::EntityDesc* outDesc) const
    {
        *outDesc = Obscura::EntityDesc{};
        if (m_Scene.HasComponent<Obscura::IDComponent>(entity))
        {
            const auto& idComp = m_Scene.GetComponent<Obscura::IDComponent>(entity);
            outDesc->uuid = idComp.uuid;
            outDesc->parentUuid = idComp.parentUuid;
            strncpy_s(outDesc->name, sizeof(outDesc->name), idComp.name.c_str(), _TRUNCATE);
        }
        if (m_Scene.HasComponent<Obscura::Transform>(entity))
        {
            const auto& t = m_Scene.GetComponent<Obscura::Transform>(entity);
            outDesc->hasTransform = true;
            outDesc->position[0] = t.position.x;
            outDesc->position[1] = t.position.y;
            outDesc->position[2] = t.position.z;
            outDesc->rotation[0] = t.rotation.x;
            outDesc->rotation[1] = t.rotation.y;
            outDesc->rotation[2] = t.rotation.z;
            outDesc->scale[0]    = t.scale.x;
            outDesc->scale[1]    = t.scale.y;
            outDesc->scale[2]    = t.scale.z;
        }
        if (m_Scene.HasComponent<Obscura::Sprite2D>(entity))
        {
            const auto& s = m_Scene.GetComponent<Obscura::Sprite2D>(entity);
            outDesc->hasSprite2D = true;
            outDesc->spriteColor[0] = s.color.r;
            outDesc->spriteColor[1] = s.color.g;
            outDesc->spriteColor[2] = s.color.b;
            outDesc->spriteColor[3] = s.color.a;
            outDesc->textureSlot = s.textureSlot;
            outDesc->useTexture  = s.useTexture;
            outDesc->uvOffset[0] = s.uvOffset.x;
            outDesc->uvOffset[1] = s.uvOffset.y;
            outDesc->uvScale[0]  = s.uvScale.x;
            outDesc->uvScale[1]  = s.uvScale.y;
            outDesc->spriteVisible = s.visible;
        }
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
