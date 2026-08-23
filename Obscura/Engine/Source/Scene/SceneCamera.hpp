#pragma once

#include "ICamera.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace Obscura
{
    class OBSCURA_ENGINE_API SceneCamera : public ICamera
    {
    public:
        enum class ProjectionType { Perspective = 0, Orthographic = 1 };

    public:
        SceneCamera();
        ~SceneCamera() override = default;

        void SetPerspective(float verticalFovRad, float nearClip, float farClip);
        void SetOrthographic(float size, float nearClip, float farClip);

        void SetViewportSize(uint32_t width, uint32_t height) override;
        [[nodiscard]] uint32_t GetViewportWidth() const noexcept override { return m_ViewportWidth; }
        [[nodiscard]] uint32_t GetViewportHeight() const noexcept override { return m_ViewportHeight; }

        void SetProjectionType(ProjectionType type);
        [[nodiscard]] ProjectionType GetProjectionType() const noexcept { return m_ProjectionType; }

        [[nodiscard]] float GetPerspectiveVerticalFOV() const noexcept { return m_PerspectiveFOV; }
        void SetPerspectiveVerticalFOV(float verticalFovRad);
        [[nodiscard]] float GetPerspectiveNearClip() const noexcept { return m_PerspectiveNear; }
        void SetPerspectiveNearClip(float nearClip);
        [[nodiscard]] float GetPerspectiveFarClip() const noexcept { return m_PerspectiveFar; }
        void SetPerspectiveFarClip(float farClip);

        [[nodiscard]] float GetOrthographicSize() const noexcept { return m_OrthographicSize; }
        void SetOrthographicSize(float size);
        [[nodiscard]] float GetOrthographicNearClip() const noexcept { return m_OrthographicNear; }
        void SetOrthographicNearClip(float nearClip);
        [[nodiscard]] float GetOrthographicFarClip() const noexcept { return m_OrthographicFar; }
        void SetOrthographicFarClip(float farClip);

        void SetViewMatrix(const glm::mat4& view) noexcept { m_ViewMatrix = view; RecalculateViewProjection(); }
        void SetPosition(const glm::vec3& position) noexcept { m_Position = position; }

        [[nodiscard]] const glm::mat4& GetProjectionMatrix() const noexcept override { return m_ProjectionMatrix; }
        [[nodiscard]] const glm::mat4& GetViewMatrix() const noexcept override { return m_ViewMatrix; }
        [[nodiscard]] const glm::mat4& GetViewProjectionMatrix() const noexcept override { return m_ViewProjectionMatrix; }
        [[nodiscard]] glm::vec3 GetPosition() const noexcept override { return m_Position; }

    private:
        void RecalculateProjection();
        void RecalculateViewProjection();

    private:
        ProjectionType m_ProjectionType = ProjectionType::Orthographic;

        float m_PerspectiveFOV  = glm::radians(45.0f);
        float m_PerspectiveNear = 0.01f;
        float m_PerspectiveFar  = 1000.0f;

        float m_OrthographicSize = 10.0f;
        float m_OrthographicNear = -1.0f;
        float m_OrthographicFar  = 1.0f;

        uint32_t m_ViewportWidth  = 1280;
        uint32_t m_ViewportHeight = 720;
        float    m_AspectRatio    = 16.0f / 9.0f;

        glm::mat4 m_ProjectionMatrix     = glm::mat4(1.0f);
        glm::mat4 m_ViewMatrix           = glm::mat4(1.0f);
        glm::mat4 m_ViewProjectionMatrix = glm::mat4(1.0f);
        glm::vec3 m_Position             = glm::vec3(0.0f);
    };
}
