#pragma once

#include <Obscura/API.hpp>
#include <Obscura/Types.hpp>

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <chrono>
#include <cstdint>
#include <limits>
#include <random>
#include <string>
#include <utility>

namespace Obscura
{
    // Generates a random 64-bit uint64_t UUID hash (non-zero)
    inline uint64_t GenerateUUID64()
    {
        static thread_local std::mt19937_64 generator(
            static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()) ^ std::random_device{}()
        );
        static thread_local std::uniform_int_distribution<uint64_t> distribution(1, std::numeric_limits<uint64_t>::max());
        return distribution(generator);
    }

    // Component: ID (64-bit uint64_t UUID hash, name, parent UUID)
    struct IDComponent
    {
        uint64_t    uuid = 0;
        std::string name = "Entity";
        uint64_t    parentUuid = 0; // 0 = root / no parent
    };

    // Component: Tag / Name
    struct TagComponent
    {
        std::string tag = "Entity";
    };

    // Component: Transform (Position, Rotation in radians, Scale)
    struct Transform
    {
        glm::vec3 position = { 0.0f, 0.0f, 0.0f };
        glm::vec3 rotation = { 0.0f, 0.0f, 0.0f }; // Euler angles in radians
        glm::vec3 scale    = { 1.0f, 1.0f, 1.0f };

        [[nodiscard]] glm::mat4 GetTransformMatrix() const noexcept
        {
            glm::mat4 mat = glm::translate(glm::mat4(1.0f), position);
            mat = glm::rotate(mat, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
            mat = glm::rotate(mat, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
            mat = glm::rotate(mat, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
            mat = glm::scale(mat, scale);
            return mat;
        }

        operator glm::mat4() const noexcept { return GetTransformMatrix(); }
    };

    // Component: 2D Sprite
    struct Sprite2D
    {
        glm::vec4 color       = glm::vec4(1.0f); // Tint RGBA
        uint32_t  textureSlot = 0;               // Bindless slot index
        bool      useTexture  = false;           // If true, sample bindless texture; else solid color
        glm::vec2 uvOffset    = glm::vec2(0.0f, 0.0f);
        glm::vec2 uvScale     = glm::vec2(1.0f, 1.0f);
        bool      visible     = true;
    };

    // EnTT-backed Scene managing entities and components.
    class OBSCURA_ENGINE_API Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;

        // Entity lifecycle
        entt::entity CreateEntity(const std::string& name = "Entity");
        entt::entity CreateEntityWithUUID(uint64_t uuid, const std::string& name = "Entity", uint64_t parentUuid = 0);
        void         DestroyEntity(entt::entity entity);
        void         Clear();

        [[nodiscard]] entt::entity FindEntityByUUID(uint64_t uuid) const
        {
            auto view = m_Registry.view<IDComponent>();
            for (auto entity : view)
            {
                if (view.get<IDComponent>(entity).uuid == uuid)
                {
                    return entity;
                }
            }
            return entt::null;
        }

        template <typename Component, typename... Args>
        Component& AddComponent(entt::entity entity, Args&&... args)
        {
            return m_Registry.emplace<Component>(entity, std::forward<Args>(args)...);
        }

        template <typename Component>
        [[nodiscard]] Component& GetComponent(entt::entity entity)
        {
            return m_Registry.get<Component>(entity);
        }

        template <typename Component>
        [[nodiscard]] const Component& GetComponent(entt::entity entity) const
        {
            return m_Registry.get<Component>(entity);
        }

        template <typename Component>
        [[nodiscard]] bool HasComponent(entt::entity entity) const
        {
            return m_Registry.all_of<Component>(entity);
        }

        template <typename Component>
        void RemoveComponent(entt::entity entity)
        {
            m_Registry.remove<Component>(entity);
        }

        [[nodiscard]] entt::registry&       GetRegistry()       noexcept { return m_Registry; }
        [[nodiscard]] const entt::registry& GetRegistry() const noexcept { return m_Registry; }

        [[nodiscard]] bool IsEmpty() const noexcept
        {
            return m_Registry.view<TagComponent>().empty();
        }

        void SetViewProj(const glm::mat4& viewProj) noexcept { m_ViewProj = viewProj; }
        [[nodiscard]] const glm::mat4& GetViewProj() const noexcept { return m_ViewProj; }

    private:
        entt::registry m_Registry;
        glm::mat4      m_ViewProj = glm::mat4(1.0f);
    };
}
