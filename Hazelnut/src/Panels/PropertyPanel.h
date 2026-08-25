#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Scene/Entity.h"
#include "Hazel/Scene/Scene.h"

#include <filesystem>

namespace Aster
{
    class PropertyPanel
    {
      public:
        void SetContext(const Hazel::Ref<Hazel::Scene>& scene);
        void SetSelectedEntity(Hazel::Entity entity);
        void SetSelectedMetaPath(const std::filesystem::path& metaPath);

        void OnImGuiRender();

      private:
        enum class SelectionType
        {
            None,
            Entity,
            Meta
        };

        template <typename T> void DisplayAddComponentEntry(const std::string& entryName);

        void DrawEntityProperties(Hazel::Entity entity);
        void DrawAssetProperties();

        Hazel::Ref<Hazel::Scene> m_Context;
        Hazel::Entity m_SelectedEntity;
        std::filesystem::path m_SelectedMetaPath;
        SelectionType m_SelectionType = SelectionType::None;
    };
} // namespace Aster
