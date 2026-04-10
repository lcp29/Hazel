//
// Created by helmholtz on 2026/4/7.
//

#pragma once
#include <glm/glm.hpp>

template <typename T, int dataSize>
struct Padded
{
    T data;
    uint8_t padding[dataSize - sizeof(T)];
};

struct PerViewUniformBufferInner
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
};

using PerViewUniformBuffer = Padded<PerViewUniformBufferInner, 256>;