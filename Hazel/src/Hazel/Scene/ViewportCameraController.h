//
// Created by helmholtz on 2026/3/22.
//

#pragma once
#include "Transform.h"
#include "glm/vec2.hpp"
#include "Hazel/Core/Timestep.h"
#include "Hazel/Renderer/Camera.h"

namespace Hazel
{
    class ViewportCameraController
    {
    public:
        ViewportCameraController() = default;

        ViewportCameraController(Camera* camera)
            : m_ViewportCamera(camera) {}

        void OnUpdate(Timestep ts);
        void StopControlling();
        bool IsControlling() const { return m_WasRightMousePressed; }

        const Camera* GetCamera() const
        {
            return m_ViewportCamera;
        }

        void SetCamera(Camera* camera)
        {
            m_ViewportCamera = camera;
        }

        const Transform& GetCameraTransform() const
        {
            return m_Transform;
        }

        void SetCameraTransform(const Transform& transform)
        {
            m_Transform = transform;
        }

    private:
        Transform m_Transform{};
        Camera* m_ViewportCamera = nullptr;
        glm::vec2 m_LastMousePosition = {0.0f, 0.0f};
        bool m_WasRightMousePressed = false;
        float m_MoveSpeed = 5.0f;
        float m_RotationSpeed = 0.0025f;
    };
} // namespace Hazel
