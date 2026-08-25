// ======== Aster Modify Begin ========
#include "ContentBrowserPanel.h"

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Asset/AssetUtils.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Project/Project.h"
#include "hzpch.h"

#include <imgui.h>

#include <cstring>
#include <fstream>

namespace Hazel
{
    ContentBrowserPanel::ContentBrowserPanel(void* directoryIcon, void* fileIcon)
        : m_BaseDirectory(Project::GetAssetDirectory())
        , m_CurrentDirectory(m_BaseDirectory)
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
            if (ImGui::Button("-", ImVec2(h, h))) { thumbnailSize = std::max(16.0, thumbnailSize - 32.0); }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(150);
            ImGui::SliderFloat("", &thumbnailSize, 16, 512, "");
            ImGui::SameLine();
            if (ImGui::Button("+", ImVec2(h, h))) { thumbnailSize = std::min(512.0, thumbnailSize + 32.0); }
            ImGui::PopID();
            ImGui::PopStyleVar();

            ImGui::EndMenuBar();
        }

        float cellSize = thumbnailSize + padding;

        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = static_cast<int>(panelWidth / cellSize);
        columnCount = std::max(columnCount, 1);

        ImGui::Columns(columnCount, nullptr, false);

        for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
        {
            const auto& path = directoryEntry.path();
            std::string filenameString = path.filename().string();

            ImGui::PushID(filenameString.c_str());
            void* icon = directoryEntry.is_directory() ? m_DirectoryIcon : m_FileIcon;
            ImVec4 buttonColor = path == m_SelectedPath ? ImVec4(0.2f, 0.35f, 0.6f, 0.35f) : ImVec4(0, 0, 0, 0);
            ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
            ImGui::ImageButton("##btn", icon, ImVec2(thumbnailSize, thumbnailSize), ImVec2(0, 0), ImVec2(1, 1));

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
                if (directoryEntry.is_directory()) m_CurrentDirectory /= path.filename();
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
                if (ImGui::MenuItem("Material")) { OpenCreateAssetPopup(Aster::AssetType::Material, "New Material"); }
                if (ImGui::MenuItem("Render Texture"))
                {
                    OpenCreateAssetPopup(Aster::AssetType::RenderTexture, "New Render Texture");
                }
                if (ImGui::MenuItem("Sampler")) { OpenCreateAssetPopup(Aster::AssetType::Sampler, "New Sampler"); }
                if (ImGui::MenuItem("Shader")) { OpenCreateAssetPopup(Aster::AssetType::Shader, "New Shader"); }
                if (ImGui::MenuItem("Compute Shader"))
                {
                    OpenCreateAssetPopup(Aster::AssetType::ComputeShader, "New Compute Shader");
                }
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }

        DrawCreateAssetPopup();

        ImGui::End();
    }

    std::filesystem::path ContentBrowserPanel::GetSelectedMetaPath() const
    {
        if (m_SelectedPath.empty()) { return {}; }

        return Aster::GetMetaPathFromAssetPath(m_SelectedPath);
    }

    std::filesystem::path ContentBrowserPanel::GetUniquePath(const std::string& baseName,
                                                             const std::string& extension) const
    {
        auto path = m_CurrentDirectory / (baseName + extension);
        if (!std::filesystem::exists(path)) { return path; }

        int index = 1;
        while (true)
        {
            path = m_CurrentDirectory / (baseName + " " + std::to_string(index) + extension);
            if (!std::filesystem::exists(path)) { return path; }
            index++;
        }
    }

    void ContentBrowserPanel::SelectPath(const std::filesystem::path& path)
    {
        m_SelectedPath = path;
        m_SelectionVersion++;
    }

    void ContentBrowserPanel::OpenCreateAssetPopup(Aster::AssetType assetType, const char* defaultName)
    {
        m_PendingAssetType = assetType;
        std::memset(m_CreateAssetNameBuffer, 0, sizeof(m_CreateAssetNameBuffer));
        std::strncpy(m_CreateAssetNameBuffer, defaultName, sizeof(m_CreateAssetNameBuffer) - 1);
        m_OpenCreateAssetPopup = true;
    }

    void ContentBrowserPanel::DrawCreateAssetPopup()
    {
        if (m_OpenCreateAssetPopup)
        {
            ImGui::OpenPopup("Create Asset");
            m_OpenCreateAssetPopup = false;
        }

        if (ImGui::BeginPopupModal("Create Asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            auto assetTypeName = "";
            switch (m_PendingAssetType)
            {
                case Aster::AssetType::Material:
                    assetTypeName = "Material";
                    break;
                case Aster::AssetType::RenderTexture:
                    assetTypeName = "Render Texture";
                    break;
                case Aster::AssetType::Sampler:
                    assetTypeName = "Sampler";
                    break;
                case Aster::AssetType::Shader:
                    assetTypeName = "Shader";
                    break;
                case Aster::AssetType::ComputeShader:
                    assetTypeName = "Compute Shader";
                    break;
                default:
                    break;
            }

            ImGui::Text("Type: %s", assetTypeName);
            ImGui::SetNextItemWidth(280.0f);
            bool create = ImGui::InputText(
                "Name", m_CreateAssetNameBuffer, sizeof(m_CreateAssetNameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SetItemDefaultFocus();

            if (ImGui::Button("Create") || create)
            {
                const std::string name = m_CreateAssetNameBuffer;
                switch (m_PendingAssetType)
                {
                    case Aster::AssetType::Material:
                        CreateMaterialAsset(name);
                        break;
                    case Aster::AssetType::RenderTexture:
                        CreateRenderTextureAsset(name);
                        break;
                    case Aster::AssetType::Sampler:
                        CreateSamplerAsset(name);
                        break;
                    case Aster::AssetType::Shader:
                        CreateShaderAsset(name);
                        break;
                    case Aster::AssetType::ComputeShader:
                        CreateComputeShaderAsset(name);
                        break;
                    default:
                        break;
                }

                m_PendingAssetType = Aster::AssetType::Unknown;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                m_PendingAssetType = Aster::AssetType::Unknown;
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                m_PendingAssetType = Aster::AssetType::Unknown;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void ContentBrowserPanel::CreateMaterialAsset(const std::string& name)
    {
        auto metaPath = GetUniquePath(name, ".mat.meta");
        auto meta = Aster::MaterialAssetMeta::CreateDefault();
        WriteMetaToFile(meta, metaPath);

        auto assetRegistryTerm =
            std::make_unique<Aster::AssetRegistryTerm>(meta.GetUUID(), Aster::AssetType::Material, metaPath);

        auto* assetManager = Project::GetActive()->GetAssetManager();
        assetManager->RegisterAsset(std::move(assetRegistryTerm));
        SelectPath(metaPath);
    }

    void ContentBrowserPanel::CreateRenderTextureAsset(const std::string& name)
    {
        auto metaPath = GetUniquePath(name, ".rt.meta");
        auto meta = Aster::RenderTextureAssetMeta::CreateDefault();
        WriteMetaToFile(meta, metaPath);

        auto* assetManager = Project::GetActive()->GetAssetManager();

        auto assetRegistryTerm =
            std::make_unique<Aster::AssetRegistryTerm>(meta.GetUUID(), Aster::AssetType::RenderTexture, metaPath);

        assetManager->RegisterAsset(std::move(assetRegistryTerm));
        SelectPath(metaPath);
    }

    void ContentBrowserPanel::CreateSamplerAsset(const std::string& name)
    {
        auto metaPath = GetUniquePath(name, ".sampler.meta");
        auto meta = Aster::SamplerAssetMeta::CreateDefault();
        WriteMetaToFile(meta, metaPath);

        auto* assetManager = Project::GetActive()->GetAssetManager();

        auto assetRegistryTerm =
            std::make_unique<Aster::AssetRegistryTerm>(meta.GetUUID(), Aster::AssetType::Sampler, metaPath);

        assetManager->RegisterAsset(std::move(assetRegistryTerm));
        SelectPath(metaPath);
    }

    void ContentBrowserPanel::CreateShaderAsset(const std::string& name)
    {
        auto sourcePath = GetUniquePath(name, ".shader");
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
        sourceOutput.close();

        auto meta = Aster::ShaderAssetMeta::CreateDefault();
        WriteMetaToFile(meta, metaPath);

        auto* assetManager = Project::GetActive()->GetAssetManager();

        auto assetRegistryTerm =
            std::make_unique<Aster::AssetRegistryTerm>(meta.GetUUID(), Aster::AssetType::Shader, sourcePath);

        assetManager->RegisterAsset(std::move(assetRegistryTerm));
        SelectPath(sourcePath);
    }

    void ContentBrowserPanel::CreateComputeShaderAsset(const std::string& name)
    {
        auto sourcePath = GetUniquePath(name, ".comp");
        auto metaPath = sourcePath;
        metaPath += ".meta";

        std::ofstream sourceOutput(sourcePath);
        sourceOutput << "#version 450 core\n\n"
                        "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n\n"
                        "void main()\n"
                        "{\n"
                        "}\n";

        auto meta = Aster::ComputeShaderAssetMeta::CreateDefault();
        WriteMetaToFile(meta, metaPath);

        auto* assetManager = Project::GetActive()->GetAssetManager();

        auto assetRegistryTerm =
            std::make_unique<Aster::AssetRegistryTerm>(meta.GetUUID(), Aster::AssetType::ComputeShader, sourcePath);

        assetManager->RegisterAsset(std::move(assetRegistryTerm));
        SelectPath(sourcePath);
    }
} // namespace Hazel

// ======== Aster Modify End ========
