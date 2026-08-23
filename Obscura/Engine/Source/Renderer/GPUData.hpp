#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace Obscura
{
    // Push constant block for 2D Sprite rendering: exactly 128 bytes.
    struct SpritePushConstants
    {
        glm::mat4 model       = glm::mat4(1.0f); // 64 bytes (offset 0)
        glm::vec4 color       = glm::vec4(1.0f); // 16 bytes (offset 64)
        glm::vec2 uvOffset    = glm::vec2(0.0f); // 8 bytes  (offset 80)
        glm::vec2 uvScale     = glm::vec2(1.0f); // 8 bytes  (offset 88)
        uint32_t  textureSlot = 0;               // 4 bytes  (offset 96)
        uint32_t  useTexture  = 0;               // 4 bytes  (offset 100)
        float     padding[6]  = { 0.0f };        // 24 bytes (offset 104)
    };
    static_assert(sizeof(SpritePushConstants) == 128, "SpritePushConstants must be exactly 128 bytes");

    // Standard MVP Push constants (128 bytes)
    struct MVPPushConstants
    {
        glm::mat4 model    = glm::mat4(1.0f); // 64 bytes
        glm::mat4 viewProj = glm::mat4(1.0f); // 64 bytes
    };
    static_assert(sizeof(MVPPushConstants) == 128, "MVPPushConstants must be exactly 128 bytes");
}
