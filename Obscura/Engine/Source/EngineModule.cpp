#include <Obscura/API.hpp>
#include <Obscura/IEngine.hpp>
#include <Obscura/IRHI.hpp>
#include <Obscura/Logger.hpp>
#include <Obscura/Module.hpp>
#include <Obscura/Types.hpp>

#include <Obscura/WorkerManager.hpp>
#include "Assets/AssetManager.hpp"
#include "Scene/Scene.hpp"
#include "Scene/EditorCamera.hpp"

#include "Renderer/SceneRenderer.hpp"

#include <filesystem>
#include <memory>
#include <algorithm>

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

        // Initialize EditorCamera (Perspective by default)
        m_EditorCamera.SetViewportSize(m_Params.WindowWidth, m_Params.WindowHeight);
        m_EditorCamera.SetPosition(glm::vec3(0.0f, 0.0f, 8.0f));

        // Initialize Engine SceneRenderer
        m_SceneRenderer = Obscura::CreateScope<Obscura::SceneRenderer>();
        if (!m_SceneRenderer->Initialize(m_RHI))
        {
            LOG_ASSERT(false, "[Engine.dll] SceneRenderer failed to initialize — scene draws will be disabled.");
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

        // Process WASD camera movement ONLY when Right Mouse Button is held
        if (m_RightMouseDown)
        {
            m_EditorCamera.ProcessKeyboard(m_KeyW, m_KeyA, m_KeyS, m_KeyD, m_KeyQ, m_KeyE, deltaTime, 5.0f);
        }
        m_EditorCamera.OnUpdate(deltaTime);

        if (m_RHI)
        {
            m_RHI->BeginFrame();

            if (m_SceneRenderer)
            {
                m_SceneRenderer->Render(&m_Scene, m_EditorCamera, m_RHI);
            }

            m_RHI->EndFrame();
        }
    }

    void HandleViewportResize(std::uint32_t width, std::uint32_t height) override
    {
        if (width == 0 || height == 0) return;
        m_EditorCamera.SetViewportSize(width, height);
        if (m_SceneRenderer)
        {
            m_SceneRenderer->OnResize(width, height, m_RHI);
        }
    }

    void HandleMouseMove(float x, float y, bool rightMouseDown, bool middleMouseDown, bool leftMouseDown) override
    {
        float dx = x - m_LastMouseX;
        float dy = y - m_LastMouseY;
        m_LastMouseX = x;
        m_LastMouseY = y;
        m_RightMouseDown = rightMouseDown;

        if (rightMouseDown)
        {
            m_EditorCamera.MouseRotate(glm::vec2(dx, dy));
        }
        else if (middleMouseDown)
        {
            m_EditorCamera.MousePan(glm::vec2(dx, dy));
        }
    }

    void HandleMouseButton(int button, bool pressed, float x, float y) override
    {
        m_LastMouseX = x;
        m_LastMouseY = y;

        // Qt::RightButton = 2, Qt::LeftButton = 1, Qt::MiddleButton = 4
        if (button == 2)
        {
            m_RightMouseDown = pressed;
        }
        else if (button == 4)
        {
            m_MiddleMouseDown = pressed;
        }
        else if (button == 1)
        {
            m_LeftMouseDown = pressed;
        }
    }

    void HandleKey(int key, bool pressed) override
    {
        // Handle Qt key codes / ASCII chars for WASD + QE
        if (key == 'W' || key == 'w' || key == 0x57) m_KeyW = pressed;
        if (key == 'A' || key == 'a' || key == 0x41) m_KeyA = pressed;
        if (key == 'S' || key == 's' || key == 0x53) m_KeyS = pressed;
        if (key == 'D' || key == 'd' || key == 0x44) m_KeyD = pressed;
        if (key == 'Q' || key == 'q' || key == 0x51) m_KeyQ = pressed;
        if (key == 'E' || key == 'e' || key == 0x45) m_KeyE = pressed;
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
    Obscura::EngineInitParams              m_Params{};
    Obscura::Module                        m_RhiModule;
    Obscura::IRHI*                         m_RHI          = nullptr;
    Obscura::DestroyRHIFn                  m_DestroyRHIFn = nullptr;
    uint32_t                               m_FrameCount   = 0;

    Obscura::Scene                         m_Scene;
    Obscura::Scope<Obscura::SceneRenderer> m_SceneRenderer;
    Obscura::EditorCamera                  m_EditorCamera;

    float                                  m_LastMouseX     = 0.0f;
    float                                  m_LastMouseY     = 0.0f;
    bool                                   m_RightMouseDown = false;
    bool                                   m_MiddleMouseDown= false;
    bool                                   m_LeftMouseDown  = false;

    bool                                   m_KeyW = false;
    bool                                   m_KeyA = false;
    bool                                   m_KeyS = false;
    bool                                   m_KeyD = false;
    bool                                   m_KeyQ = false;
    bool                                   m_KeyE = false;
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
