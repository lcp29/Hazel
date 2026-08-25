// Declares render scene.
// Created: 2026-03-29.

#pragma once
#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"

#include <glm/glm.hpp>
#include <map>
#include <unordered_set>

namespace Hazel
{
    class Renderer;
}

namespace Aster
{

    struct RenderObject
    {
        glm::mat4x4 transform{1.0f};
        Hazel::UUID material = Hazel::UUID(-1);
        Hazel::UUID shader = Hazel::UUID(-1);
        Hazel::UUID mesh = Hazel::UUID(-1);
        Hazel::UUID entity = Hazel::UUID(-1);
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

        Hazel::UUID entity = Hazel::UUID(-1);
        Type type = Type::Add;

        union
        {
            struct
            {
                glm::mat4x4 transform;
            } changeTransform;

            struct
            {
                Hazel::UUID material;
            } changeMaterial;

            struct
            {
                Hazel::UUID mesh;
                uint32_t meshInstanceID = 0;
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
        RenderScene(Hazel::Renderer* renderer);
        void Update(const std::vector<RenderSceneUpdatePayload>& payload);
        void Clear();
        void SortRenderObjectShader();

        const std::multimap<Hazel::UUID, RenderObject*>& GetRenderObjectsSortedByShader() const
        { return m_RenderObjectsSortedByShader; }

      private:
        Hazel::Renderer* m_Renderer = nullptr;

        std::mutex m_RenderObjectsMutex;
        std::unordered_map<Hazel::UUID, std::unique_ptr<RenderObject>> m_RenderObjects;

        std::unordered_map<RenderObject*, std::multimap<Hazel::UUID, RenderObject*>::iterator>
            m_RenderObjectLocationInMap;
        std::multimap<Hazel::UUID, RenderObject*> m_RenderObjectsSortedByShader;
        std::unordered_set<RenderObject*> m_RenderObjectsUnsorted;
    };
} // namespace Aster
