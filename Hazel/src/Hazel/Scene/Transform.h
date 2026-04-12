//
// Created by helmholtz on 2026/4/9.
//

#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Hazel
{
    struct Transform
    {
        glm::vec3 translation = {0.0f, 0.0f, 0.0f};
        glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
        glm::vec3 scale = {1.0f, 1.0f, 1.0f};

        Transform() = default;
        Transform(const Transform&) = default;

        Transform(const glm::vec3& translation)
            : translation(translation)
        {}

        Transform(const glm::mat4& transform) { SetTransform(transform); }

        glm::mat4 GetTransform() const
        {
            glm::mat4 rotationMatrix = glm::toMat4(glm::quat(rotation));

            return glm::translate(glm::mat4(1.0f), translation) * rotationMatrix * glm::scale(glm::mat4(1.0f), scale);
        }

        glm::mat4 GetView() const { return glm::inverse(GetTransform()); }

        void SetTransform(const glm::mat4& transform)
        {
            glm::quat orientation;
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(transform, scale, orientation, translation, skew, perspective);
            rotation = glm::eulerAngles(orientation);
        }
    };
} // namespace Hazel