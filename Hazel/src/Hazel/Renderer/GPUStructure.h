//
// Created by helmholtz on 2026/4/7.
//

#pragma once
#include <glm/glm.hpp>

struct PerViewUniformBuffer
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
    glm::vec3 cameraPos;

    char _padding[52];
};