#pragma once

#include "ICamera.hpp"

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <utility>

namespace Obscura
{
    class OBSCURA_ENGINE_API EditorCamera : public ICamera
    {
    public:
        EditorCamera();
        EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);
        ~EditorCamera() override = default;

        void OnUpdate(float deltaTime);
        void ProcessKeyboard(bool w, bool a, bool s, bool d, bool q, bool e, float deltaTime, float speed = 5.0f);

        void MousePan(const glm::vec2& delta);
        void MouseRotate(const glm::vec2& delta);
        void MouseZoom(float delta);

        void SetViewportSize(uint32_t width, uint32_t height) override;
        [[nodiscard]] uint32_t GetViewportWidth() const noexcept override { return m_ViewportWidth; }
        [[nodiscard]] uint32_t GetViewportHeight() const noexcept override { return m_ViewportHeight; }

        [[nodiscard]] const glm::mat4& GetProjectionMatrix() const noexcept override { return m_ProjectionMatrix; }
        [[nodiscard]] const glm::mat4& GetViewMatrix() const noexcept override { return m_ViewMatrix; }
        [[nodiscard]] const glm::mat4& GetViewProjectionMatrix() const noexcept override { return m_ViewProjectionMatrix; }

        [[nodiscard]] glm::vec3 GetPosition() const noexcept override { return m_Position; }
        [[nodiscard]] glm::vec3 GetFocalPoint() const noexcept { return m_FocalPoint; }
        [[nodiscard]] float GetDistance() const noexcept { return m_Distance; }
        [[nodiscard]] float GetPitch() const noexcept { return m_Pitch; }
        [[nodiscard]] float GetYaw() const noexcept { return m_Yaw; }

        void SetFocalPoint(const glm::vec3& focalPoint) { m_FocalPoint = focalPoint; UpdateView(); }
        void SetDistance(float distance) { m_Distance = distance; UpdateView(); }
        void SetPosition(const glm::vec3& position) { m_Position = position; m_FocalPoint = position + GetForwardDirection() * m_Distance; UpdateView(); }

        [[nodiscard]] glm::vec3 GetUpDirection() const;
        [[nodiscard]] glm::vec3 GetRightDirection() const;
        [[nodiscard]] glm::vec3 GetForwardDirection() const;
        [[nodiscard]] glm::quat GetOrientation() const;

    private:
        void UpdateProjection();
        void UpdateView();

        [[nodiscard]] std::pair<float, float> PanSpeed() const;
        [[nodiscard]] float RotationSpeed() const noexcept { return 0.003f; }
        [[nodiscard]] float ZoomSpeed() const;

    private:
        float m_FOV         = 45.0f;
        float m_AspectRatio = 1.778f;
        float m_NearClip    = 0.1f;
        float m_FarClip     = 1000.0f;

        uint32_t m_ViewportWidth  = 1280;
        uint32_t m_ViewportHeight = 720;

        glm::mat4 m_ProjectionMatrix     = glm::mat4(1.0f);
        glm::mat4 m_ViewMatrix           = glm::mat4(1.0f);
        glm::mat4 m_ViewProjectionMatrix = glm::mat4(1.0f);

        glm::vec3 m_Position   = { 0.0f, 0.0f, 5.0f };
        glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };

        float m_Pitch    = 0.0f;
        float m_Yaw      = 0.0f;
        float m_Distance = 5.0f;
    };
}
