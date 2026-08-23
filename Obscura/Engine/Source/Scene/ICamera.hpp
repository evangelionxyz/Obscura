#pragma once

#include <Obscura/API.hpp>
#include <glm/glm.hpp>
#include <cstdint>

namespace Obscura
{
    class OBSCURA_ENGINE_API ICamera
    {
    public:
        virtual ~ICamera() = default;

        [[nodiscard]] virtual const glm::mat4& GetProjectionMatrix() const noexcept = 0;
        [[nodiscard]] virtual const glm::mat4& GetViewMatrix() const noexcept = 0;
        [[nodiscard]] virtual const glm::mat4& GetViewProjectionMatrix() const noexcept = 0;
        [[nodiscard]] virtual glm::vec3 GetPosition() const noexcept = 0;

        virtual void SetViewportSize(uint32_t width, uint32_t height) = 0;
        [[nodiscard]] virtual uint32_t GetViewportWidth() const noexcept = 0;
        [[nodiscard]] virtual uint32_t GetViewportHeight() const noexcept = 0;
    };
}
