//
// Created by helmholtz on 2026/3/20.
//

#include "Camera.h"

#include "Hazel/Project/GlobalSettingRegistry.h"

namespace Hazel
{
    namespace
    {
        float GetDefaultPerspectiveFovX()
        {
            return GlobalSettings.Get(Camera::DefaultPerspectiveFovXString, Camera::DefaultPerspectiveFovX);
        }

        float GetDefaultPerspectiveNear()
        {
            return GlobalSettings.Get(Camera::DefaultPerspectiveNearString, Camera::DefaultPerspectiveNear);
        }

        float GetDefaultPerspectiveFar()
        {
            return GlobalSettings.Get(Camera::DefaultPerspectiveFarString, Camera::DefaultPerspectiveFar);
        }

        float GetDefaultOrthoWidth()
        {
            return GlobalSettings.Get(Camera::DefaultOrthoWidthString, Camera::DefaultOrthoWidth);
        }

        float GetDefaultOrthoNear()
        {
            return GlobalSettings.Get(Camera::DefaultOrthoNearString, Camera::DefaultOrthoNear);
        }

        float GetDefaultOrthoFar()
        {
            return GlobalSettings.Get(Camera::DefaultOrthoFarString, Camera::DefaultOrthoFar);
        }
    } // namespace

    Camera::Camera()
        : m_FovX(GetDefaultPerspectiveFovX())
        , m_NearClip(GetDefaultPerspectiveNear())
        , m_FarClip(GetDefaultPerspectiveFar())
        , m_OrthoWidth(GetDefaultOrthoWidth())
        , m_OrthoNearClip(GetDefaultOrthoNear())
        , m_OrthoFarClip(GetDefaultOrthoFar())
    {
        UpdateMatrices();
    }

    Camera Camera::Perspective(float fovX, float aspectRatio, float nearClip, float farClip)
    {
        Camera camera(fovX,
                      aspectRatio,
                      nearClip,
                      farClip,
                      GetDefaultOrthoWidth(),
                      1.0f,
                      GetDefaultOrthoNear(),
                      GetDefaultOrthoFar());
        camera.m_ProjectionType = ProjectionType::Perspective;
        camera.UpdateMatrices();
        return camera;
    }

    Camera Camera::Perspective(const glm::mat4& projection)
    {
        Camera camera(projection);
        camera.m_ProjectionType = ProjectionType::Perspective;
        camera.UpdateVectors();
        return camera;
    }

    Camera Camera::Orthographic(float orthoWidth, float orthoAspectRatio, float orthoNearClip, float orthoFarClip)
    {
        Camera camera(GetDefaultPerspectiveFovX(),
                      1.0f,
                      GetDefaultPerspectiveNear(),
                      GetDefaultPerspectiveFar(),
                      orthoWidth,
                      orthoAspectRatio,
                      orthoNearClip,
                      orthoFarClip);
        camera.m_ProjectionType = ProjectionType::Orthographic;
        camera.UpdateMatrices();
        return camera;
    }

    Camera Camera::Orthographic(const glm::mat4& projection)
    {
        Camera camera(projection);
        camera.m_ProjectionType = ProjectionType::Orthographic;
        camera.UpdateVectors();
        return camera;
    }

    void Camera::SetViewportSize(uint32_t width, uint32_t height)
    {
        m_ViewportSize = {width, height};
        UpdateMatrices();
    }

    void Camera::SetViewportSizeKeepFovY(uint32_t width, uint32_t height)
    {
        m_ViewportSize = {width, height};
        if (m_ProjectionType == ProjectionType::Perspective)
        {
            float fovY = 2.0f * atan(tan(m_FovX * 0.5f) / m_AspectRatio);
            float newAspectRatio = static_cast<float>(width) / static_cast<float>(height);
            float newFovX = 2.0f * atan(tan(fovY * 0.5f) * newAspectRatio);
            m_FovX = newFovX;
            m_AspectRatio = newAspectRatio;
        }
        else if (m_ProjectionType == ProjectionType::Orthographic)
        {
            float orthoHeight = m_OrthoWidth / m_OrthoAspectRatio;
            float newAspectRatio = static_cast<float>(width) / static_cast<float>(height);
            float newOrthoWidth = orthoHeight * newAspectRatio;
            m_OrthoWidth = newOrthoWidth;
            m_OrthoAspectRatio = newAspectRatio;
        }
        UpdateMatrices();
    }

    void Camera::SetFovX(float fovX)
    {
        m_FovX = fovX;
        UpdateMatrices();
    }

    void Camera::SetAspectRatio(float aspectRatio)
    {
        m_AspectRatio = aspectRatio;
        UpdateMatrices();
    }

    void Camera::SetNearClip(float nearClip)
    {
        m_NearClip = nearClip;
        UpdateMatrices();
    }

    void Camera::SetFarClip(float farClip)
    {
        m_FarClip = farClip;
        UpdateMatrices();
    }

    void Camera::SetOrthoWidth(float orthoWidth)
    {
        m_OrthoWidth = orthoWidth;
        UpdateMatrices();
    }

    void Camera::SetOrthoAspectRatio(float orthoAspectRatio)
    {
        m_OrthoAspectRatio = orthoAspectRatio;
        UpdateMatrices();
    }

    void Camera::SetOrthoNearClip(float orthoNearClip)
    {
        m_OrthoNearClip = orthoNearClip;
        UpdateMatrices();
    }

    void Camera::SetOrthoFarClip(float orthoFarClip)
    {
        m_OrthoFarClip = orthoFarClip;
        UpdateMatrices();
    }

    YAML::Node Camera::Serialize() const
    {
        YAML::Node node;
        node["ProjectionType"] = m_ProjectionType == ProjectionType::Perspective ? "Perspective" : "Orthographic";
        if (m_ProjectionType == ProjectionType::Perspective)
        {
            node["FovX"] = m_FovX;
            node["AspectRatio"] = m_AspectRatio;
            node["NearClip"] = m_NearClip;
            node["FarClip"] = m_FarClip;
        }
        else if (m_ProjectionType == ProjectionType::Orthographic)
        {
            node["OrthoWidth"] = m_OrthoWidth;
            node["OrthoAspectRatio"] = m_OrthoAspectRatio;
            node["OrthoNearClip"] = m_OrthoNearClip;
            node["OrthoFarClip"] = m_OrthoFarClip;
        }
        return node;
    }

    Camera Camera::Deserialize(const YAML::Node& node)
    {
        ProjectionType projectionType = node["ProjectionType"].as<std::string>() == "Perspective"
                                            ? ProjectionType::Perspective
                                            : ProjectionType::Orthographic;
        if (projectionType == ProjectionType::Perspective)
        {
            float fovX = node["FovX"].as<float>();
            float aspectRatio = node["AspectRatio"].as<float>();
            float nearClip = node["NearClip"].as<float>();
            float farClip = node["FarClip"].as<float>();
            return Perspective(fovX, aspectRatio, nearClip, farClip);
        }
        float orthoWidth = node["OrthoWidth"].as<float>();
        float orthoAspectRatio = node["OrthoAspectRatio"].as<float>();
        float orthoNearClip = node["OrthoNearClip"].as<float>();
        float orthoFarClip = node["OrthoFarClip"].as<float>();
        return Orthographic(orthoWidth, orthoAspectRatio, orthoNearClip, orthoFarClip);
    }

    void Camera::UpdateMatrices()
    {
        if (m_ProjectionType == ProjectionType::Perspective)
        {
            float fovY = 2.0f * atan(tan(m_FovX * 0.5f) / m_AspectRatio);
            m_Projection = glm::perspective(fovY, m_AspectRatio, m_NearClip, m_FarClip);
        }
        else
        {
            float orthoLeft = -m_OrthoWidth * 0.5f;
            float orthoRight = m_OrthoWidth * 0.5f;
            float orthoBottom = orthoLeft / m_OrthoAspectRatio;
            float orthoTop = orthoRight / m_OrthoAspectRatio;
            m_Projection = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_OrthoNearClip, m_OrthoFarClip);
        }
    }

    void Camera::UpdateVectors()
    {
        if (m_ProjectionType == ProjectionType::Perspective)
        {
            m_FovX = 2.0f * atan(1 / m_Projection[0][0]);
            m_AspectRatio = m_Projection[1][1] / m_Projection[0][0];
            m_NearClip = m_Projection[3][2] / (m_Projection[2][2] - 1);
            m_FarClip = m_Projection[3][2] / (m_Projection[2][2] + 1);
        }
        else
        {
            m_OrthoWidth = 2.0f / m_Projection[0][0];
            m_OrthoAspectRatio = m_Projection[1][1] / m_Projection[0][0];
            m_OrthoNearClip = (m_Projection[3][2] + 1) / m_Projection[2][2];
            m_OrthoFarClip = (m_Projection[3][2] - 1) / m_Projection[2][2];
        }
    }
} // namespace Hazel