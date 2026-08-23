#include "SceneCamera.hpp"

#include <algorithm>

namespace Obscura
{
    SceneCamera::SceneCamera()
    {
        RecalculateProjection();
    }

    void SceneCamera::SetPerspective(float verticalFovRad, float nearClip, float farClip)
    {
        m_ProjectionType  = ProjectionType::Perspective;
        m_PerspectiveFOV  = verticalFovRad;
        m_PerspectiveNear = nearClip;
        m_PerspectiveFar  = farClip;
        RecalculateProjection();
    }

    void SceneCamera::SetOrthographic(float size, float nearClip, float farClip)
    {
        m_ProjectionType   = ProjectionType::Orthographic;
        m_OrthographicSize = size;
        m_OrthographicNear = nearClip;
        m_OrthographicFar  = farClip;
        RecalculateProjection();
    }

    void SceneCamera::SetViewportSize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        m_ViewportWidth  = width;
        m_ViewportHeight = height;
        m_AspectRatio    = static_cast<float>(width) / static_cast<float>(height);
        RecalculateProjection();
    }

    void SceneCamera::SetProjectionType(ProjectionType type)
    {
        m_ProjectionType = type;
        RecalculateProjection();
    }

    void SceneCamera::SetPerspectiveVerticalFOV(float verticalFovRad)
    {
        m_PerspectiveFOV = verticalFovRad;
        RecalculateProjection();
    }

    void SceneCamera::SetPerspectiveNearClip(float nearClip)
    {
        m_PerspectiveNear = nearClip;
        RecalculateProjection();
    }

    void SceneCamera::SetPerspectiveFarClip(float farClip)
    {
        m_PerspectiveFar = farClip;
        RecalculateProjection();
    }

    void SceneCamera::SetOrthographicSize(float size)
    {
        m_OrthographicSize = size;
        RecalculateProjection();
    }

    void SceneCamera::SetOrthographicNearClip(float nearClip)
    {
        m_OrthographicNear = nearClip;
        RecalculateProjection();
    }

    void SceneCamera::SetOrthographicFarClip(float farClip)
    {
        m_OrthographicFar = farClip;
        RecalculateProjection();
    }

    void SceneCamera::RecalculateProjection()
    {
        if (m_ProjectionType == ProjectionType::Perspective)
        {
            m_ProjectionMatrix = glm::perspectiveZO(m_PerspectiveFOV, m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
            m_ProjectionMatrix[1][1] *= -1.0f; // Vulkan NDC Y-flip
        }
        else
        {
            float orthoLeft   = -m_OrthographicSize * m_AspectRatio * 0.5f;
            float orthoRight  =  m_OrthographicSize * m_AspectRatio * 0.5f;
            float orthoBottom = -m_OrthographicSize * 0.5f;
            float orthoTop    =  m_OrthographicSize * 0.5f;

            m_ProjectionMatrix = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_OrthographicNear, m_OrthographicFar);
            m_ProjectionMatrix[1][1] *= -1.0f; // Vulkan NDC Y-flip
        }

        RecalculateViewProjection();
    }

    void SceneCamera::RecalculateViewProjection()
    {
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }
}
