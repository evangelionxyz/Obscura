#include "Scene.hpp"

namespace Obscura
{
    entt::entity Scene::CreateEntity(const std::string& name)
    {
        entt::entity entity = m_Registry.create();
        uint64_t uuid = GenerateUUID64();
        m_Registry.emplace<IDComponent>(entity, uuid, name, 0ull);
        m_Registry.emplace<TagComponent>(entity, name);
        m_Registry.emplace<Transform>(entity);
        return entity;
    }

    entt::entity Scene::CreateEntityWithUUID(uint64_t uuid, const std::string& name, uint64_t parentUuid)
    {
        entt::entity entity = m_Registry.create();
        if (uuid == 0)
        {
            uuid = GenerateUUID64();
        }
        m_Registry.emplace<IDComponent>(entity, uuid, name, parentUuid);
        m_Registry.emplace<TagComponent>(entity, name);
        m_Registry.emplace<Transform>(entity);
        return entity;
    }

    void Scene::DestroyEntity(entt::entity entity)
    {
        if (m_Registry.valid(entity))
        {
            m_Registry.destroy(entity);
        }
    }

    void Scene::Clear()
    {
        m_Registry.clear();
    }
}
