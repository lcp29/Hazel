#pragma once

// ======== Aster Modify Begin ========
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Asset/MaterialAsset.h"
#include "Hazel/Asset/SamplerAsset.h"

#include <filesystem>

// ======== Aster Modify End ========

namespace Hazel
{
    class ContentBrowserPanel
    {
      public:
        // ======== Aster Modify Begin ========
        ContentBrowserPanel(void* directoryIcon, void* fileIcon);
        // ======== Aster Modify End ========

        void OnImGuiRender();

        // ======== Aster Modify Begin ========
        const std::filesystem::path& GetSelectedPath() const { return m_SelectedPath; }

        std::filesystem::path GetSelectedMetaPath() const;

        uint64_t GetSelectionVersion() const { return m_SelectionVersion; }

      private:
        std::filesystem::path GetUniquePath(const std::string& baseName, const std::string& extension) const;
        void SelectPath(const std::filesystem::path& path);
        void OpenCreateAssetPopup(Aster::AssetType assetType, const char* defaultName);
        void DrawCreateAssetPopup();
        void CreateMaterialAsset(const std::string& name);
        void CreateRenderTextureAsset(const std::string& name);
        void CreateSamplerAsset(const std::string& name);
        void CreateShaderAsset(const std::string& name);
        void CreateComputeShaderAsset(const std::string& name);

        std::filesystem::path m_BaseDirectory;
        std::filesystem::path m_CurrentDirectory;
        std::filesystem::path m_SelectedPath;

        void* m_DirectoryIcon = nullptr;
        void* m_FileIcon = nullptr;
        uint64_t m_SelectionVersion = 0;
        Aster::AssetType m_PendingAssetType = Aster::AssetType::Unknown;
        char m_CreateAssetNameBuffer[256] = {};
        bool m_OpenCreateAssetPopup = false;
    };
} // namespace Hazel

// ======== Aster Modify End ========
