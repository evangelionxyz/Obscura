#include "Transform.hpp"

namespace Obscura
{
    const glm::mat4 Transform::GetTransformMatrix() const
    {
        return glm::translate(glm::mat4(1.0f), translation)
            * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
    }
}
