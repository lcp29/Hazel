//
// Created by helmholtz on 2026/3/22.
//

#pragma once
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

        const Camera* GetCamera() const
        {
            return m_ViewportCamera;
        }

    private:
        Camera* m_ViewportCamera;
    };
} // namespace Hazel