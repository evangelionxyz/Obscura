#include "Serializer.hpp"
#include <Obscura/Logger.hpp>
#include <json/json.hpp>
#include <fstream>

namespace Obscura
{
    using json = nlohmann::ordered_json;

    SceneSerializer::SceneSerializer(Scene* scene)
        : m_Scene(scene)
    {
    }

    bool SceneSerializer::SerializeToString(std::string& outJsonString)
    {
        if (!m_Scene)
        {
            LOG_ERROR("[SceneSerializer] Scene pointer is null!");
            return false;
        }

        json root = json::object();
        root["Scene"] = "ObscuraScene";
        root["Version"] = "1.0";

        json entitiesArray = json::array();

        auto& registry = m_Scene->GetRegistry();
        auto view = registry.view<IDComponent>();

        for (auto entity : view)
        {
            const auto& id = view.get<IDComponent>(entity);

            json entityObj = json::object();
            entityObj["UUID"] = id.uuid;
            entityObj["Name"] = id.name;
            entityObj["ParentUUID"] = id.parentUuid;

            if (m_Scene->HasComponent<TagComponent>(entity))
            {
                const auto& tag = m_Scene->GetComponent<TagComponent>(entity);
                entityObj["TagComponent"] = {
                    { "Tag", tag.tag }
                };
            }

            if (m_Scene->HasComponent<Transform>(entity))
            {
                const auto& tf = m_Scene->GetComponent<Transform>(entity);
                entityObj["Transform"] = {
                    { "Position", { tf.position.x, tf.position.y, tf.position.z } },
                    { "Rotation", { tf.rotation.x, tf.rotation.y, tf.rotation.z } },
                    { "Scale",    { tf.scale.x, tf.scale.y, tf.scale.z } }
                };
            }

            if (m_Scene->HasComponent<Sprite2D>(entity))
            {
                const auto& sp = m_Scene->GetComponent<Sprite2D>(entity);
                entityObj["Sprite2D"] = {
                    { "Color",         { sp.color.r, sp.color.g, sp.color.b, sp.color.a } },
                    { "TextureHandle", sp.textureHandle },
                    { "TexturePath",   sp.texturePath },
                    { "UVOffset",      { sp.uvOffset.x, sp.uvOffset.y } },
                    { "UVScale",       { sp.uvScale.x, sp.uvScale.y } },
                    { "Visible",       sp.visible }
                };
            }

            entitiesArray.push_back(entityObj);
        }

        root["Entities"] = entitiesArray;
        outJsonString = root.dump(4);
        return true;
    }

    bool SceneSerializer::Serialize(const std::filesystem::path& filepath)
    {
        std::string jsonStr;
        if (!SerializeToString(jsonStr))
        {
            return false;
        }

        std::ofstream fout(filepath);
        if (!fout.is_open())
        {
            LOG_ERROR("[SceneSerializer] Failed to open file for writing: {}", filepath.string());
            return false;
        }

        fout << jsonStr;
        fout.close();
        LOG_INFO("[SceneSerializer] Scene successfully serialized to: {}", filepath.string());
        return true;
    }

    bool SceneSerializer::DeserializeFromString(const std::string& jsonString)
    {
        if (!m_Scene)
        {
            LOG_ERROR("[SceneSerializer] Scene pointer is null!");
            return false;
        }

        try
        {
            json root = json::parse(jsonString);

            if (!root.contains("Entities") || !root["Entities"].is_array())
            {
                LOG_ERROR("[SceneSerializer] Invalid scene JSON: 'Entities' array missing");
                return false;
            }

            m_Scene->Clear();

            for (const auto& entityObj : root["Entities"])
            {
                uint64_t uuid = entityObj.value("UUID", 0ULL);
                std::string name = entityObj.value("Name", "Entity");
                uint64_t parentUuid = entityObj.value("ParentUUID", 0ULL);

                auto entity = m_Scene->CreateEntityWithUUID(uuid, name, parentUuid);

                if (entityObj.contains("TagComponent") && entityObj["TagComponent"].is_object())
                {
                    const auto& tagObj = entityObj["TagComponent"];
                    if (m_Scene->HasComponent<TagComponent>(entity))
                    {
                        m_Scene->GetComponent<TagComponent>(entity).tag = tagObj.value("Tag", name);
                    }
                    else
                    {
                        m_Scene->AddComponent<TagComponent>(entity, tagObj.value("Tag", name));
                    }
                }

                if (entityObj.contains("Transform") && entityObj["Transform"].is_object())
                {
                    const auto& tfObj = entityObj["Transform"];
                    auto& tf = m_Scene->HasComponent<Transform>(entity) 
                        ? m_Scene->GetComponent<Transform>(entity)
                        : m_Scene->AddComponent<Transform>(entity);

                    if (tfObj.contains("Position") && tfObj["Position"].is_array() && tfObj["Position"].size() >= 3)
                    {
                        tf.position = glm::vec3(
                            tfObj["Position"][0].get<float>(),
                            tfObj["Position"][1].get<float>(),
                            tfObj["Position"][2].get<float>()
                        );
                    }
                    if (tfObj.contains("Rotation") && tfObj["Rotation"].is_array() && tfObj["Rotation"].size() >= 3)
                    {
                        tf.rotation = glm::vec3(
                            tfObj["Rotation"][0].get<float>(),
                            tfObj["Rotation"][1].get<float>(),
                            tfObj["Rotation"][2].get<float>()
                        );
                    }
                    if (tfObj.contains("Scale") && tfObj["Scale"].is_array() && tfObj["Scale"].size() >= 3)
                    {
                        tf.scale = glm::vec3(
                            tfObj["Scale"][0].get<float>(),
                            tfObj["Scale"][1].get<float>(),
                            tfObj["Scale"][2].get<float>()
                        );
                    }
                }

                if (entityObj.contains("Sprite2D") && entityObj["Sprite2D"].is_object())
                {
                    const auto& spObj = entityObj["Sprite2D"];
                    auto& sp = m_Scene->HasComponent<Sprite2D>(entity)
                        ? m_Scene->GetComponent<Sprite2D>(entity)
                        : m_Scene->AddComponent<Sprite2D>(entity);

                    if (spObj.contains("Color") && spObj["Color"].is_array() && spObj["Color"].size() >= 4)
                    {
                        sp.color = glm::vec4(
                            spObj["Color"][0].get<float>(),
                            spObj["Color"][1].get<float>(),
                            spObj["Color"][2].get<float>(),
                            spObj["Color"][3].get<float>()
                        );
                    }
                    sp.textureHandle = spObj.value("TextureHandle", 0ULL);
                    sp.texturePath = spObj.value("TexturePath", "");
                    if (spObj.contains("UVOffset") && spObj["UVOffset"].is_array() && spObj["UVOffset"].size() >= 2)
                    {
                        sp.uvOffset = glm::vec2(
                            spObj["UVOffset"][0].get<float>(),
                            spObj["UVOffset"][1].get<float>()
                        );
                    }
                    if (spObj.contains("UVScale") && spObj["UVScale"].is_array() && spObj["UVScale"].size() >= 2)
                    {
                        sp.uvScale = glm::vec2(
                            spObj["UVScale"][0].get<float>(),
                            spObj["UVScale"][1].get<float>()
                        );
                    }
                    sp.visible = spObj.value("Visible", true);
                }
            }

            LOG_INFO("[SceneSerializer] Scene successfully deserialized. Entities loaded: {}", root["Entities"].size());
            return true;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("[SceneSerializer] JSON parsing error: {}", e.what());
            return false;
        }
    }

    bool SceneSerializer::Deserialize(const std::filesystem::path& filepath)
    {
        std::ifstream fin(filepath);
        if (!fin.is_open())
        {
            LOG_ERROR("[SceneSerializer] Failed to open file for reading: {}", filepath.string());
            return false;
        }

        std::string jsonString((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
        fin.close();

        return DeserializeFromString(jsonString);
    }
}
