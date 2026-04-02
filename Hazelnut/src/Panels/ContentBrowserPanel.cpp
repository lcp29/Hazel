#include "hzpch.h"
#include "ContentBrowserPanel.h"

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Project/Project.h"

#include <imgui.h>
#include <fstream>

namespace Hazel
{
    namespace
    {
        template <typename T>
        void WriteMetaFile(const std::filesystem::path& path, const T& meta)
        {
            std::ofstream output(path);
            output << meta.Serialize();
        }
    }

    ContentBrowserPanel::ContentBrowserPanel(void* directoryIcon, void* fileIcon)
        : m_BaseDirectory(Project::GetAssetDirectory()), m_CurrentDirectory(m_BaseDirectory)
    {
        m_DirectoryIcon = directoryIcon;
        m_FileIcon = fileIcon;
    }

    void ContentBrowserPanel::OnImGuiRender()
    {
        ImGui::Begin("Content Browser", nullptr, ImGuiWindowFlags_MenuBar);

        static float thumbnailSize = 128.0f;
        static float padding = 16.0f;

        if (ImGui::BeginMenuBar())
        {
            bool canGoUp = m_CurrentDirectory != std::filesystem::path(m_BaseDirectory);
            ImGui::BeginDisabled(!canGoUp);
            if (ImGui::ArrowButton("ButtonAssetBrowserGoUp", ImGuiDir_Left))
            {
                m_CurrentDirectory = m_CurrentDirectory.parent_path();
            }
            ImGui::EndDisabled();

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 4.0f));
            float h = ImGui::GetFrameHeight();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 150 - 2 * h);
            ImGui::PushID("ContentIconSizeSlider");
            if (ImGui::Button("-", ImVec2(h, h)))
            {
                thumbnailSize = std::max(16.0, thumbnailSize - 32.0);
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(150);
            ImGui::SliderFloat("", &thumbnailSize, 16, 512, "");
            ImGui::SameLine();
            if (ImGui::Button("+", ImVec2(h, h)))
            {
                thumbnailSize = std::min(512.0, thumbnailSize + 32.0);
            }
            ImGui::PopID();
            ImGui::PopStyleVar();

            ImGui::EndMenuBar();
        }

        float cellSize = thumbnailSize + padding;

        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = static_cast<int>(panelWidth / cellSize);
        if (columnCount < 1)
            columnCount = 1;

        ImGui::Columns(columnCount, nullptr, false);

        for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
        {
            const auto& path = directoryEntry.path();
            std::string filenameString = path.filename().string();

            ImGui::PushID(filenameString.c_str());
            void* icon = directoryEntry.is_directory() ? m_DirectoryIcon : m_FileIcon;
            ImVec4 buttonColor = path == m_SelectedPath ? ImVec4(0.2f, 0.35f, 0.6f, 0.35f) : ImVec4(0, 0, 0, 0);
            ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
            ImGui::ImageButton("##btn",
                               icon,
                               ImVec2(thumbnailSize, thumbnailSize),
                               ImVec2(0, 0),
                               ImVec2(1, 1));

            if (ImGui::BeginDragDropSource())
            {
                std::filesystem::path relativePath(path);
                const wchar_t* itemPath = relativePath.c_str();
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath, (wcslen(itemPath) + 1) * sizeof(wchar_t));
                ImGui::EndDragDropSource();
            }

            ImGui::PopStyleColor();
            if (ImGui::IsItemClicked())
            {
                m_SelectedPath = path;
                m_SelectionVersion++;
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                if (directoryEntry.is_directory())
                    m_CurrentDirectory /= path.filename();
            }
            ImGui::TextWrapped(filenameString.c_str());

            ImGui::NextColumn();

            ImGui::PopID();
        }

        ImGui::Columns(1);

        if (ImGui::BeginPopupContextWindow("ContentBrowserCreateAsset", ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::BeginMenu("Create"))
            {
                if (ImGui::MenuItem("Material"))
                {
                    CreateMaterialAsset();
                }
                if (ImGui::MenuItem("Render Texture"))
                {
                    CreateRenderTextureAsset();
                }
                if (ImGui::MenuItem("Sampler"))
                {
                    CreateSamplerAsset();
                }
                if (ImGui::MenuItem("Shader"))
                {
                    CreateShaderAsset();
                }
                if (ImGui::MenuItem("Compute Shader"))
                {
                    CreateComputeShaderAsset();
                }
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    std::filesystem::path ContentBrowserPanel::GetSelectedMetaPath() const
    {
        return m_SelectedPath.extension() == ".meta" ? m_SelectedPath : std::filesystem::path{};
    }

    std::filesystem::path ContentBrowserPanel::GetUniquePath(const std::string& baseName, const std::string& extension) const
    {
        auto path = m_CurrentDirectory / (baseName + extension);
        if (!std::filesystem::exists(path))
        {
            return path;
        }

        int index = 1;
        while (true)
        {
            path = m_CurrentDirectory / (baseName + " " + std::to_string(index) + extension);
            if (!std::filesystem::exists(path))
            {
                return path;
            }
            index++;
        }
    }

    void ContentBrowserPanel::SelectPath(const std::filesystem::path& path)
    {
        m_SelectedPath = path;
        m_SelectionVersion++;
    }

    void ContentBrowserPanel::CreateMaterialAsset()
    {
        auto metaPath = GetUniquePath("New Material", ".mat.meta");
        MaterialAssetMeta meta{};
        meta.uuid = UUID();
        meta.shader = UUID(-1);
        WriteMetaFile(metaPath, meta);

        auto* assetManager = Project::GetActive()->GetAssetManager();
        assetManager->AddAsset<MaterialAsset>(
            meta.uuid,
            meta.uuid,
            assetManager,
            Application::Get().GetRenderer(),
            metaPath,
            meta);
        SelectPath(metaPath);
    }

    void ContentBrowserPanel::CreateRenderTextureAsset()
    {
        auto metaPath = GetUniquePath("New Render Texture", ".rt.meta");
        RenderTextureAssetMeta meta{};
        meta.uuid = UUID();
        WriteMetaFile(metaPath, meta);

        auto* assetManager = Project::GetActive()->GetAssetManager();
        assetManager->AddAsset<RenderTextureAsset>(meta.uuid,
                                                   meta.uuid,
                                                   metaPath,
                                                   Application::Get().GetRenderer(),
                                                   meta);
        SelectPath(metaPath);
    }

    void ContentBrowserPanel::CreateSamplerAsset()
    {
        auto metaPath = GetUniquePath("New Sampler", ".sampler.meta");
        SamplerAssetMeta meta{};
        meta.uuid = UUID();
        WriteMetaFile(metaPath, meta);

        auto* assetManager = Project::GetActive()->GetAssetManager();
        assetManager->AddAsset<SamplerAsset>(meta.uuid,
                                             meta.uuid,
                                             metaPath,
                                             Application::Get().GetRenderer(),
                                             meta);
        SelectPath(metaPath);
    }

    void ContentBrowserPanel::CreateShaderAsset()
    {
        auto sourcePath = GetUniquePath("New Shader", ".shader");
        auto metaPath = sourcePath;
        metaPath += ".meta";

        std::ofstream sourceOutput(sourcePath);
        sourceOutput << "#version 450 core\n\n"
                        "#ifdef VERTEX_SHADER\n"
                        "layout(location = 0) out vec2 v_UV;\n"
                        "vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));\n"
                        "vec2 uvs[3] = vec2[](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));\n"
                        "void main()\n"
                        "{\n"
                        "    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);\n"
                        "    v_UV = uvs[gl_VertexIndex];\n"
                        "}\n"
                        "#endif\n\n"
                        "#ifdef FRAGMENT_SHADER\n"
                        "layout(location = 0) in vec2 v_UV;\n"
                        "layout(location = 0) out vec4 o_Color;\n"
                        "void main()\n"
                        "{\n"
                        "    o_Color = vec4(v_UV, 0.0, 1.0);\n"
                        "}\n"
                        "#endif\n";

        ShaderAssetMeta meta{};
        meta.uuid = UUID();
        WriteMetaFile(metaPath, meta);

        auto* assetManager = Project::GetActive()->GetAssetManager();
        assetManager->AddAsset<ShaderAsset>(meta.uuid, Application::Get().GetRenderer(), sourcePath, meta);
        SelectPath(sourcePath);
    }

    void ContentBrowserPanel::CreateComputeShaderAsset()
    {
        auto sourcePath = GetUniquePath("New Compute Shader", ".comp");
        auto metaPath = sourcePath;
        metaPath += ".meta";

        std::ofstream sourceOutput(sourcePath);
        sourceOutput << "#version 450 core\n\n"
                        "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n\n"
                        "void main()\n"
                        "{\n"
                        "}\n";

        ComputeShaderAssetMeta meta{};
        meta.uuid = UUID();
        WriteMetaFile(metaPath, meta);

        auto* assetManager = Project::GetActive()->GetAssetManager();
        assetManager->AddAsset<ComputeShaderAsset>(meta.uuid, Application::Get().GetRenderer(), sourcePath, meta);
        SelectPath(sourcePath);
    }
}
