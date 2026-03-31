#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Scene/Entity.h"
#include "Hazel/Scene/Scene.h"

#include <filesystem>

namespace Hazel
{
    class PropertyPanel
    {
    public:
        void SetContext(const Ref<Scene>& scene);
        void SetSelectedEntity(Entity entity);
        void SetSelectedMetaPath(const std::filesystem::path& metaPath);

        void OnImGuiRender();

    private:
        enum class SelectionType
        {
            None,
            Entity,
            Meta
        };

        template <typename T>
        void DisplayAddComponentEntry(const std::string& entryName);

        void DrawEntityProperties(Entity entity);
        void DrawAssetProperties();

    private:
        Ref<Scene> m_Context;
        Entity m_SelectedEntity;
        std::filesystem::path m_SelectedMetaPath;
        SelectionType m_SelectionType = SelectionType::None;
    };
}
