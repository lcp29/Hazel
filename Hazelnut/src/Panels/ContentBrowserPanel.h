#pragma once

#include "Hazel/Asset/MaterialAsset.h"
#include "Hazel/Asset/RenderTextureAsset.h"
#include "Hazel/Asset/SamplerAsset.h"
#include "Hazel/Asset/ShaderAsset.h"
#include "Hazel/Asset/ComputeShaderAsset.h"

#include <filesystem>

namespace Hazel
{
    class ContentBrowserPanel
    {
    public:
        ContentBrowserPanel(void* directoryIcon, void* fileIcon);

        void OnImGuiRender();
        const std::filesystem::path& GetSelectedPath() const { return m_SelectedPath; }
        std::filesystem::path GetSelectedMetaPath() const;
        uint64_t GetSelectionVersion() const { return m_SelectionVersion; }

    private:
        std::filesystem::path GetUniquePath(const std::string& baseName, const std::string& extension) const;
        void SelectPath(const std::filesystem::path& path);
        void CreateMaterialAsset();
        void CreateRenderTextureAsset();
        void CreateSamplerAsset();
        void CreateShaderAsset();
        void CreateComputeShaderAsset();

        std::filesystem::path m_BaseDirectory;
        std::filesystem::path m_CurrentDirectory;
        std::filesystem::path m_SelectedPath;

        void* m_DirectoryIcon = nullptr;
        void* m_FileIcon = nullptr;
        uint64_t m_SelectionVersion = 0;
    };
}
