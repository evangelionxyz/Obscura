#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace Obscura
{
    // Match SPIR-V reflection of default.vert.glsl (36 bytes stride):
    // Location 0: vec3 inPosition (offset 0, 12 bytes)
    // Location 1: vec2 inTexCoord (offset 12, 8 bytes)
    // Location 2: vec4 inColor    (offset 20, 16 bytes)
    struct Quad2DVertex
    {
        glm::vec3 position = glm::vec3(0.0f); // 12 bytes (offset 0)
        glm::vec2 uv       = glm::vec2(0.0f); // 8 bytes  (offset 12)
        glm::vec4 color    = glm::vec4(1.0f); // 16 bytes (offset 20)
    };
    static_assert(sizeof(Quad2DVertex) == 36, "Quad2DVertex layout must be 36 bytes matching default.vert.glsl reflection");
}
