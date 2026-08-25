#pragma once

#include "Hazel/Core/Base.h"
// ======== Aster Modify Begin ========
#include "Hazel/Scene/Entity.h"
#include "Hazel/Scene/Scene.h"

// ======== Aster Modify End ========

namespace Hazel
{
    class SceneHierarchyPanel
    {
      public:
        SceneHierarchyPanel() = default;
        SceneHierarchyPanel(const Ref<Scene>& scene);

        void SetContext(const Ref<Scene>& scene);

        void OnImGuiRender();

        Entity GetSelectedEntity() const { return m_SelectionContext; }

        void SetSelectedEntity(Entity entity);

        // ======== Aster Modify Begin ========
        uint64_t GetSelectionVersion() const { return m_SelectionVersion; }

      private:
        // ======== Aster Modify End ========
        void DrawEntityNode(Entity entity);

        Ref<Scene> m_Context;
        Entity m_SelectionContext;
        // ======== Aster Modify Begin ========
        uint64_t m_SelectionVersion = 0;
        std::vector<Entity> m_EntityDeletionQueue;
        // ======== Aster Modify End ========
    };
} // namespace Hazel