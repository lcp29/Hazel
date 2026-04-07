//
// Created by helmholtz on 2026/3/29.
//

#pragma once
#include "Hazel/RHI/RHI.h"
#include "Hazel/Core/UUID.h"

#include <glm/glm.hpp>

namespace Hazel
{
    class Renderer;

    struct RenderObject
    {
        glm::mat4x4 transform{1.0f};
        UUID material = UUID(-1);
        UUID mesh = UUID(-1);
        UUID entity = UUID(-1);
    };

    struct RenderSceneUpdatePayload
    {
        enum class Type
        {
            ChangeTransform,
            ChangeMaterial,
            ChangeMesh,
            Add,
            Remove
        };

        UUID entity = UUID(-1);
        Type type = Type::Add;

        union
        {
            struct
            {
                glm::mat4x4 transform;
            } changeTransform;

            struct
            {
                UUID material;
            } changeMaterial;

            struct
            {
                UUID mesh;
                uint32_t meshInstanceID;
            } changeMesh;

            struct
            {
                RenderObject renderObject{};
            } add{};
        };
    };

    class RenderScene
    {
    public:
        RenderScene(Renderer* renderer);
        void Update(const std::vector<RenderSceneUpdatePayload>& payload);

    private:
        Renderer* m_Renderer = nullptr;

        std::mutex m_RenderObjectsMutex;
        std::unordered_map<UUID, RenderObject> m_RenderObjects;
    };
} // Hazel