#pragma once

#include <Obscura/API.hpp>
#include <Scene/Scene.hpp>

#include <filesystem>
#include <string>

namespace Obscura
{
    class OBSCURA_ENGINE_API SceneSerializer
    {
    public:
        explicit SceneSerializer(Scene* scene);

        bool Serialize(const std::filesystem::path& filepath);
        bool SerializeToString(std::string& outJsonString);

        bool Deserialize(const std::filesystem::path& filepath);
        bool DeserializeFromString(const std::string& jsonString);

    private:
        Scene* m_Scene = nullptr;
    };
}
