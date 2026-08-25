#include "SceneHierarchyPanel.h"

#include "Hazel/Scene/Components.h"

#include <imgui.h>

namespace Hazel
{
    SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context) { SetContext(context); }

    void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
    {
        m_Context = context;
        m_SelectionContext = {};
        // ======== Aster Modify Begin ========
        m_SelectionVersion++;
        // ======== Aster Modify End ========
    }

    void SceneHierarchyPanel::OnImGuiRender()
    {
        ImGui::Begin("Scene Hierarchy");

        if (m_Context)
        {
            // ======== Aster Modify Begin ========
            m_EntityDeletionQueue.clear();

            for (auto& enttEntity : m_Context->m_Registry.view<entt::entity>())
            {
                auto& relationshipComponent = m_Context->m_Registry.get<EntityRelationshipComponent>(enttEntity);
                // only initiate root entities
                if (relationshipComponent.parent == entt::null)
                {
                    Entity entity{enttEntity, m_Context.get()};
                    DrawEntityNode(entity);
                }
            }

            for (auto& entity : m_EntityDeletionQueue)
            {
                m_Context->DestroyEntity(entity);
                if (m_SelectionContext == entity) { m_SelectionContext = {}; }
                m_SelectionVersion++;
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
            {
                m_SelectionContext = {};
                m_SelectionVersion++;
            }

            // Right-click on blank space
            if (ImGui::BeginPopupContextWindow(nullptr,
                                               ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            // ======== Aster Modify End ========
            {
                if (ImGui::MenuItem("Create Empty Entity")) m_Context->CreateEntity("Empty Entity");

                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }

    void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
    {
        m_SelectionContext = entity;
        // ======== Aster Modify Begin ========
        m_SelectionVersion++;
        // ======== Aster Modify End ========
    }

    void SceneHierarchyPanel::DrawEntityNode(Entity entity)
    {
        // ======== Aster Modify Begin ========
        auto& tag = entity.GetComponent<TagComponent>().tag;
        auto& relationship = entity.GetComponent<EntityRelationshipComponent>();

        ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0)
                                   | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        if (relationship.childCount == 0) { flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen; }

        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        bool opened = ImGui::TreeNodeEx((void*)static_cast<uint64_t>((uint32_t)entity), flags, tag.c_str());
        // ======== Aster Modify End ========
        if (ImGui::IsItemClicked())
        {
            m_SelectionContext = entity;
            // ======== Aster Modify Begin ========
            m_SelectionVersion++;
            // ======== Aster Modify End ========
        }

        bool entityDeleted = false;
        // ======== Aster Modify Begin ========
        bool newEntityCreated = false;
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Create Entity")) { newEntityCreated = true; }

            if (ImGui::MenuItem("Delete Entity")) { entityDeleted = true; }

            ImGui::EndPopup();
        }

        if (opened)
        {
            if (relationship.childCount > 0)
            {
                auto currentChild = relationship.firstChild;
                while (currentChild != entt::null)
                {
                    Entity childEntity{currentChild, m_Context.get()};
                    DrawEntityNode(childEntity);
                    auto& childRelationship = childEntity.GetComponent<EntityRelationshipComponent>();
                    currentChild = childRelationship.nextSibling;
                }
            }
            if (relationship.childCount > 0) { ImGui::TreePop(); }
        }

        if (newEntityCreated)
        {
            auto newEntity = m_Context->CreateEntity("Empty Entity");
            auto& newEntityRelationship = m_Context->m_Registry.get<EntityRelationshipComponent>(newEntity);
            newEntityRelationship.parent = entity;
            if (relationship.firstChild != entt::null)
            {
                auto& firstChildRelationship =
                    Entity{relationship.firstChild, m_Context.get()}.GetComponent<EntityRelationshipComponent>();
                firstChildRelationship.prevSibling = static_cast<entt::entity>(newEntity);
                newEntityRelationship.nextSibling = relationship.firstChild;
            }
            relationship.firstChild = static_cast<entt::entity>(newEntity);
            relationship.childCount++;
        }

        if (entityDeleted) { m_EntityDeletionQueue.push_back(entity); }
        // ======== Aster Modify End ========
    }
} // namespace Hazel