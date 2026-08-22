#include "Scene.hpp"

namespace Obscura
{
    entt::entity Scene::CreateEntity(const std::string& name)
    {
        entt::entity entity = m_Registry.create();
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
