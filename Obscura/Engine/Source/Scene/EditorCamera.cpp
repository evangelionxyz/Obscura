#include "EditorCamera.hpp"

#include <algorithm>
#include <cmath>

namespace Obscura
{
    EditorCamera::EditorCamera()
    {
        UpdateProjection();
        UpdateView();
    }

    EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
        : m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip)
    {
        UpdateProjection();
        UpdateView();
    }

    void EditorCamera::OnUpdate(float deltaTime)
    {
        UpdateView();
    }

    void EditorCamera::ProcessKeyboard(bool w, bool a, bool s, bool d, bool q, bool e, float deltaTime, float speed)
    {
        glm::vec3 moveDir(0.0f);
        glm::vec3 forward = GetForwardDirection();
        glm::vec3 right   = GetRightDirection();
        glm::vec3 up      = GetUpDirection();

        if (w) moveDir += forward;
        if (s) moveDir -= forward;
        if (d) moveDir += right;
        if (a) moveDir -= right;
        if (e) moveDir += up;
        if (q) moveDir -= up;

        if (glm::length(moveDir) > 0.0001f)
        {
            glm::vec3 deltaPos = glm::normalize(moveDir) * speed * deltaTime;
            m_Position   += deltaPos;
            m_FocalPoint += deltaPos;
            UpdateView();
        }
    }

    void EditorCamera::SetViewportSize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        m_ViewportWidth  = width;
        m_ViewportHeight = height;
        m_AspectRatio    = static_cast<float>(width) / static_cast<float>(height);
        UpdateProjection();
    }

    void EditorCamera::MousePan(const glm::vec2& delta)
    {
        auto [xSpeed, ySpeed] = PanSpeed();
        glm::vec3 deltaPos = -GetRightDirection() * delta.x * xSpeed * m_Distance + GetUpDirection() * delta.y * ySpeed * m_Distance;
        m_Position   += deltaPos;
        m_FocalPoint += deltaPos;
        UpdateView();
    }

    void EditorCamera::MouseRotate(const glm::vec2& delta)
    {
        float yawSign = GetUpDirection().y < 0.0f ? -1.0f : 1.0f;
        m_Yaw   += yawSign * delta.x * RotationSpeed();
        m_Pitch += delta.y * RotationSpeed();
        m_Pitch  = std::clamp(m_Pitch, -1.55f, 1.55f);

        m_FocalPoint = m_Position + GetForwardDirection() * m_Distance;
        UpdateView();
    }

    void EditorCamera::MouseZoom(float delta)
    {
        m_Distance -= delta * ZoomSpeed();
        if (m_Distance < 0.5f)
        {
            m_Distance = 0.5f;
        }
        m_Position = m_FocalPoint - GetForwardDirection() * m_Distance;
        UpdateView();
    }

    glm::vec3 EditorCamera::GetUpDirection() const
    {
        return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 EditorCamera::GetRightDirection() const
    {
        return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 EditorCamera::GetForwardDirection() const
    {
        return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::quat EditorCamera::GetOrientation() const
    {
        return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
    }

    void EditorCamera::UpdateProjection()
    {
        m_AspectRatio = static_cast<float>(m_ViewportWidth) / static_cast<float>(m_ViewportHeight);
        m_ProjectionMatrix = glm::perspectiveZO(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
        m_ProjectionMatrix[1][1] *= -1.0f; // Vulkan Y-flip
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }

    void EditorCamera::UpdateView()
    {
        m_Position = m_FocalPoint - GetForwardDirection() * m_Distance;

        glm::quat orientation = GetOrientation();
        m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
        m_ViewMatrix = glm::inverse(m_ViewMatrix);

        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }

    std::pair<float, float> EditorCamera::PanSpeed() const
    {
        float x = std::min(static_cast<float>(m_ViewportWidth) / 1000.0f, 2.4f);
        float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

        float y = std::min(static_cast<float>(m_ViewportHeight) / 1000.0f, 2.4f);
        float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

        return { xFactor, yFactor };
    }

    float EditorCamera::ZoomSpeed() const
    {
        float distance = m_Distance * 0.2f;
        distance = std::max(distance, 0.0f);
        float speed = distance * distance;
        return std::min(speed, 100.0f);
    }
}
