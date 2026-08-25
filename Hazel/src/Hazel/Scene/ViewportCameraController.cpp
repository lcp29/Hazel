// Implements editor viewport camera controls.
// Created: 2026-03-22.

#include "ViewportCameraController.h"

#include "Hazel/Core/Application.h"
#include "Hazel/Core/Input.h"
#include "Hazel/Project/GlobalSettingRegistry.h"

#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Aster
{
    ViewportCameraController::ViewportCameraController()
        : m_MoveSpeed(GlobalSettings.Get(MoveSpeedString, DefaultMoveSpeed))
        , m_RotationSpeed(GlobalSettings.Get(RotationSpeedString, DefaultRotationSpeed))
    {}

    ViewportCameraController::ViewportCameraController(Hazel::Camera* camera)
        : m_ViewportCamera(camera)
        , m_MoveSpeed(GlobalSettings.Get(MoveSpeedString, DefaultMoveSpeed))
        , m_RotationSpeed(GlobalSettings.Get(RotationSpeedString, DefaultRotationSpeed))
    {}

    void ViewportCameraController::OnUpdate(Hazel::Timestep ts)
    {
        const glm::vec2 mousePosition = Hazel::Input::GetMousePosition();
        const bool rightMousePressed = Hazel::Input::IsMouseButtonPressed(Hazel::Mouse::ButtonRight);

        if (!rightMousePressed)
        {
            m_LastMousePosition = mousePosition;
            StopControlling();
            return;
        }

        if (!m_WasRightMousePressed)
        {
            m_LastMousePosition = mousePosition;
            m_WasRightMousePressed = true;
            Hazel::Application::Get().GetWindow().SetCursorMode(Hazel::CursorMode::Disabled);
        }

        const glm::vec2 mouseDelta = mousePosition - m_LastMousePosition;
        m_LastMousePosition = mousePosition;

        m_Transform.rotation.y -= mouseDelta.x * m_RotationSpeed;
        m_Transform.rotation.x -= mouseDelta.y * m_RotationSpeed;
        m_Transform.rotation.x =
            glm::clamp(m_Transform.rotation.x, -glm::half_pi<float>() + 0.01f, glm::half_pi<float>() - 0.01f);

        const glm::mat4 rotationMatrix = glm::toMat4(glm::quat(m_Transform.rotation));
        const glm::vec3 forward = glm::normalize(glm::vec3(rotationMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
        const glm::vec3 right = glm::normalize(glm::vec3(rotationMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));

        glm::vec3 movement{0.0f};
        if (Hazel::Input::IsKeyPressed(Hazel::Key::W)) { movement += forward; }
        if (Hazel::Input::IsKeyPressed(Hazel::Key::S)) { movement -= forward; }
        if (Hazel::Input::IsKeyPressed(Hazel::Key::A)) { movement -= right; }
        if (Hazel::Input::IsKeyPressed(Hazel::Key::D)) { movement += right; }

        if (movement != glm::vec3(0.0f))
        {
            m_Transform.translation += glm::normalize(movement) * m_MoveSpeed * static_cast<float>(ts);
        }
    }

    void ViewportCameraController::StopControlling()
    {
        if (!m_WasRightMousePressed) { return; }

        m_WasRightMousePressed = false;
        Hazel::Application::Get().GetWindow().SetCursorMode(Hazel::CursorMode::Normal);
    }
} // namespace Aster
