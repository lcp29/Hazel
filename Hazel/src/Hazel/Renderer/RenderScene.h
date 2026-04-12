//
// Created by helmholtz on 2026/3/29.
//

#pragma once
#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"

#include <glm/glm.hpp>
#include <map>
#include <unordered_set>

namespace Hazel
{
    class Renderer;

    struct RenderObject
    {
        glm::mat4x4 transform{1.0f};
        UUID material = UUID(-1);
        UUID shader = UUID(-1);
        UUID mesh = UUID(-1);
        UUID entity = UUID(-1);
        uint32_t enttEntity = static_cast<uint32_t>(-1);
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
        void Clear();
        void SortRenderObjectShader();

        const std::multimap<UUID, RenderObject*>& GetRenderObjectsSortedByShader() const
        {
            return m_RenderObjectsSortedByShader;
        }

      private:
        Renderer* m_Renderer = nullptr;

        std::mutex m_RenderObjectsMutex;
        std::unordered_map<UUID, std::unique_ptr<RenderObject>> m_RenderObjects;

        std::unordered_map<RenderObject*, std::multimap<UUID, RenderObject*>::iterator> m_RenderObjectLocationInMap;
        std::multimap<UUID, RenderObject*> m_RenderObjectsSortedByShader;
        std::unordered_set<RenderObject*> m_RenderObjectsUnsorted;
    };
} // namespace Hazel