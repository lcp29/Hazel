#pragma once

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

namespace Hazel
{
    class Camera
    {
    public:
        constexpr static auto DefaultPerspectiveFovXString = "r.Camera.DefaultPerspectiveFovX";
        constexpr static auto DefaultPerspectiveNearString = "r.Camera.DefaultPerspectiveNear";
        constexpr static auto DefaultPerspectiveFarString = "r.Camera.DefaultPerspectiveFar";
        constexpr static auto DefaultOrthoWidthString = "r.Camera.DefaultOrthoWidth";
        constexpr static auto DefaultOrthoNearString = "r.Camera.DefaultOrthoNear";
        constexpr static auto DefaultOrthoFarString = "r.Camera.DefaultOrthoFar";

        constexpr static float DefaultPerspectiveFovX = 0.78539816339f;
        constexpr static float DefaultPerspectiveNear = 0.01f;
        constexpr static float DefaultPerspectiveFar = 1000.0f;
        constexpr static float DefaultOrthoWidth = 10.0f;
        constexpr static float DefaultOrthoNear = 0.1f;
        constexpr static float DefaultOrthoFar = 100.0f;

        enum class ProjectionType
        {
            Perspective = 0,
            Orthographic = 1
        };

        Camera();

        static Camera Perspective(float fovX, float aspectRatio, float nearClip, float farClip);
        static Camera Perspective(const glm::mat4& projection);

        static Camera Orthographic(float orthoWidth, float orthoAspectRatio, float orthoNearClip, float orthoFarClip);
        static Camera Orthographic(const glm::mat4& projection);

        const glm::mat4& GetProjection() const
        {
            return m_Projection;
        }

        ProjectionType GetProjectionType() const
        {
            return m_ProjectionType;
        }

        void SetProjectionType(ProjectionType type)
        {
            m_ProjectionType = type;
            UpdateMatrices();
        };

        void SetViewportSize(uint32_t width, uint32_t height);
        void SetViewportSizeKeepFovY(uint32_t width, uint32_t height);

        const glm::ivec2& GetViewportSize() const
        {
            return m_ViewportSize;
        }

        void SetFovX(float fovX);
        void SetAspectRatio(float aspectRatio);
        void SetNearClip(float nearClip);
        void SetFarClip(float farClip);

        void SetOrthoWidth(float orthoWidth);
        void SetOrthoAspectRatio(float orthoAspectRatio);
        void SetOrthoNearClip(float orthoNearClip);
        void SetOrthoFarClip(float orthoFarClip);

        float GetFovX() const
        {
            return m_FovX;
        }

        float GetAspectRatio() const
        {
            return m_AspectRatio;
        }

        float GetNearClip() const
        {
            return m_NearClip;
        }

        float GetFarClip() const
        {
            return m_FarClip;
        }

        float GetOrthoWidth() const
        {
            return m_OrthoWidth;
        }

        float GetOrthoAspectRatio() const
        {
            return m_OrthoAspectRatio;
        }

        float GetOrthoNearClip() const
        {
            return m_OrthoNearClip;
        }

        float GetOrthoFarClip() const
        {
            return m_OrthoFarClip;
        }

        YAML::Node Serialize() const;
        static Camera Deserialize(const YAML::Node& node);

    private:
        // convention: x-right, y-up, z-backward

        Camera(float fovX,
               float aspectRatio,
               float nearClip,
               float farClip,
               float orthoWidth,
               float orthoAspectRatio,
               float orthoNearClip,
               float orthoFarClip)
            : m_FovX(fovX)
              , m_AspectRatio(aspectRatio)
              , m_NearClip(nearClip)
              , m_FarClip(farClip)
              , m_OrthoWidth(orthoWidth)
              , m_OrthoAspectRatio(orthoAspectRatio)
              , m_OrthoNearClip(orthoNearClip)
              , m_OrthoFarClip(orthoFarClip) {}

        Camera(const glm::mat4& projection)
            : m_Projection(projection) {}

        // functions for updating one representation from another
        void UpdateMatrices();
        void UpdateVectors();

        ProjectionType m_ProjectionType = ProjectionType::Perspective;

        // parameter representation
        // for perspective cameras
        float m_FovX = DefaultPerspectiveFovX;
        float m_AspectRatio = 1.0f; // width / height
        float m_NearClip = DefaultPerspectiveNear;
        float m_FarClip = DefaultPerspectiveFar;

        // for orthographic cameras
        float m_OrthoWidth = DefaultOrthoWidth;
        float m_OrthoAspectRatio = 1.0f; // width / height
        float m_OrthoNearClip = DefaultOrthoNear;
        float m_OrthoFarClip = DefaultOrthoFar;

        // projection matrix representation
        glm::mat4 m_Projection = glm::perspective(DefaultPerspectiveFovX, 1.0f, DefaultPerspectiveNear,
                                                  DefaultPerspectiveFar);

        // viewport information
        glm::ivec2 m_ViewportSize{800, 800};
    };
} // namespace Hazel
