#include "hzpch.h"
#include "ContentBrowserPanel.h"

#include "Hazel/Project/Project.h"

#include <imgui.h>

namespace Hazel
{
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

        ImGui::End();
    }

    std::filesystem::path ContentBrowserPanel::GetSelectedMetaPath() const
    {
        return m_SelectedPath.extension() == ".meta" ? m_SelectedPath : std::filesystem::path{};
    }
}