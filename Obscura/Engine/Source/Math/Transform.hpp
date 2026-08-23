#pragma once

#include <Obscura/API.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Obscura
{
    struct OBSCURA_ENGINE_API Transform
    {
        glm::vec3 translation = glm::vec3(0.0f);
        glm::vec3 scale       = glm::vec3(1.0f);
        glm::quat rotation    = glm::identity<glm::quat>();

        [[nodiscard]] const glm::mat4 GetTransformMatrix() const;

        operator glm::mat4() const noexcept { return GetTransformMatrix(); }
    };
}
