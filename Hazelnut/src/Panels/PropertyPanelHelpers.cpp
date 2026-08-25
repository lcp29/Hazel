#include "PropertyPanelHelpers.h"

#include <imgui.h>

#include <imgui_internal.h>

namespace Aster::PropertyPanelHelpers
{
    void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth)
    {
        ImGuiIO& io = ImGui::GetIO();
        auto boldFont = io.Fonts->Fonts[0];

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        float lineHeight = GImGui->Font->LegacySize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + 3.0f, std::max(lineHeight, ImGui::GetFrameHeight())};

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
        ImGui::PushFont(boldFont);
        if (ImGui::Button("X", buttonSize)) values.x = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Y", buttonSize)) values.y = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Z", buttonSize)) values.z = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();
        ImGui::Columns(1);
        ImGui::PopID();
    }

    const char* GetFormatName(RHIFormat format)
    {
        switch (format)
        {
            case RHIFormat::Undefined:
                return "Undefined";
            case RHIFormat::R8UNorm:
                return "R8UNorm";
            case RHIFormat::R32SInt:
                return "R32SInt";
            case RHIFormat::RG8UNorm:
                return "RG8UNorm";
            case RHIFormat::R32SFloat:
                return "R32SFloat";
            case RHIFormat::RG32SFloat:
                return "RG32SFloat";
            case RHIFormat::RGB32SFloat:
                return "RGB32SFloat";
            case RHIFormat::RG16UNorm:
                return "RG16UNorm";
            case RHIFormat::BGRA8UNorm:
                return "BGRA8UNorm";
            case RHIFormat::BGRA8SRGB:
                return "BGRA8SRGB";
            case RHIFormat::RGBA8UNorm:
                return "RGBA8UNorm";
            case RHIFormat::RGBA8SRGB:
                return "RGBA8SRGB";
            case RHIFormat::RGB10A2UNorm:
                return "RGB10A2UNorm";
            case RHIFormat::RGBA16SFloat:
                return "RGBA16SFloat";
            case RHIFormat::D32SFloat:
                return "D32SFloat";
            case RHIFormat::D32SFloatS8Uint:
                return "D32SFloatS8Uint";
            case RHIFormat::S8Uint:
                return "S8Uint";
        }

        return "Undefined";
    }

    bool DrawFormatCombo(const char* label, RHIFormat& format)
    {
        static constexpr RHIFormat formats[] = {RHIFormat::R8UNorm,
                                                RHIFormat::R32SInt,
                                                RHIFormat::RG8UNorm,
                                                RHIFormat::R32SFloat,
                                                RHIFormat::RG32SFloat,
                                                RHIFormat::RGB32SFloat,
                                                RHIFormat::RG16UNorm,
                                                RHIFormat::BGRA8UNorm,
                                                RHIFormat::BGRA8SRGB,
                                                RHIFormat::RGBA8UNorm,
                                                RHIFormat::RGBA8SRGB,
                                                RHIFormat::RGB10A2UNorm,
                                                RHIFormat::RGBA16SFloat,
                                                RHIFormat::D32SFloat,
                                                RHIFormat::D32SFloatS8Uint,
                                                RHIFormat::S8Uint};

        bool changed = false;
        if (ImGui::BeginCombo(label, GetFormatName(format)))
        {
            for (auto value : formats)
            {
                bool selected = value == format;
                if (ImGui::Selectable(GetFormatName(value), selected))
                {
                    format = value;
                    changed = true;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        return changed;
    }

    const char* GetPolygonModeName(RHIPolygonMode polygonMode)
    {
        switch (polygonMode)
        {
            case RHIPolygonMode::Fill:
                return "Fill";
            case RHIPolygonMode::Line:
                return "Line";
        }

        return "Fill";
    }

    bool DrawPolygonModeCombo(const char* label, RHIPolygonMode& polygonMode)
    {
        static constexpr RHIPolygonMode polygonModes[] = {RHIPolygonMode::Fill, RHIPolygonMode::Line};

        bool changed = false;
        if (ImGui::BeginCombo(label, GetPolygonModeName(polygonMode)))
        {
            for (auto value : polygonModes)
            {
                bool selected = value == polygonMode;
                if (ImGui::Selectable(GetPolygonModeName(value), selected))
                {
                    polygonMode = value;
                    changed = true;
                }
                if (selected) { ImGui::SetItemDefaultFocus(); }
            }
            ImGui::EndCombo();
        }

        return changed;
    }

    const char* GetCullModeName(RHICullMode cullMode)
    {
        switch (cullMode)
        {
            case RHICullMode::None:
                return "None";
            case RHICullMode::Front:
                return "Front";
            case RHICullMode::Back:
                return "Back";
        }

        return "Back";
    }

    bool DrawCullModeCombo(const char* label, RHICullMode& cullMode)
    {
        static constexpr RHICullMode cullModes[] = {RHICullMode::None, RHICullMode::Front, RHICullMode::Back};

        bool changed = false;
        if (ImGui::BeginCombo(label, GetCullModeName(cullMode)))
        {
            for (auto value : cullModes)
            {
                bool selected = value == cullMode;
                if (ImGui::Selectable(GetCullModeName(value), selected))
                {
                    cullMode = value;
                    changed = true;
                }
                if (selected) { ImGui::SetItemDefaultFocus(); }
            }
            ImGui::EndCombo();
        }

        return changed;
    }

    const char* GetViewTypeName(RHIImageViewType viewType)
    {
        switch (viewType)
        {
            case Image1D:
                return "Image1D";
            case Image2D:
                return "Image2D";
            case Image3D:
                return "Image3D";
            case Cube:
                return "Cube";
            case Image1DArray:
                return "Image1DArray";
            case Image2DArray:
                return "Image2DArray";
            case CubeArray:
                return "CubeArray";
        }

        return "Image2D";
    }

    bool DrawViewTypeCombo(const char* label, RHIImageViewType& viewType)
    {
        static constexpr RHIImageViewType viewTypes[] = {
            Image1D, Image2D, Image3D, Cube, Image1DArray, Image2DArray, CubeArray};

        bool changed = false;
        if (ImGui::BeginCombo(label, GetViewTypeName(viewType)))
        {
            for (auto value : viewTypes)
            {
                bool selected = value == viewType;
                if (ImGui::Selectable(GetViewTypeName(value), selected))
                {
                    viewType = value;
                    changed = true;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        return changed;
    }

    const char* GetFilterName(RHISamplerFilter filter)
    { return filter == RHISamplerFilter::Nearest ? "Nearest" : "Linear"; }

    bool DrawFilterCombo(const char* label, RHISamplerFilter& filter)
    {
        static constexpr RHISamplerFilter filters[] = {RHISamplerFilter::Nearest, RHISamplerFilter::Linear};

        bool changed = false;
        if (ImGui::BeginCombo(label, GetFilterName(filter)))
        {
            for (auto value : filters)
            {
                bool selected = value == filter;
                if (ImGui::Selectable(GetFilterName(value), selected))
                {
                    filter = value;
                    changed = true;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        return changed;
    }

    const char* GetAddressModeName(RHISamplerAddressMode addressMode)
    {
        switch (addressMode)
        {
            case RHISamplerAddressMode::Repeat:
                return "Repeat";
            case RHISamplerAddressMode::MirroredRepeat:
                return "MirroredRepeat";
            case RHISamplerAddressMode::ClampToEdge:
                return "ClampToEdge";
            case RHISamplerAddressMode::ClampToBorder:
                return "ClampToBorder";
        }

        return "Repeat";
    }

    bool DrawAddressModeCombo(const char* label, RHISamplerAddressMode& addressMode)
    {
        static constexpr RHISamplerAddressMode modes[] = {RHISamplerAddressMode::Repeat,
                                                          RHISamplerAddressMode::MirroredRepeat,
                                                          RHISamplerAddressMode::ClampToEdge,
                                                          RHISamplerAddressMode::ClampToBorder};

        bool changed = false;
        if (ImGui::BeginCombo(label, GetAddressModeName(addressMode)))
        {
            for (auto value : modes)
            {
                bool selected = value == addressMode;
                if (ImGui::Selectable(GetAddressModeName(value), selected))
                {
                    addressMode = value;
                    changed = true;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        return changed;
    }

    const char* GetCompareOpName(RHICompareOp compareOp)
    {
        switch (compareOp)
        {
            case RHICompareOp::Never:
                return "Never";
            case RHICompareOp::Less:
                return "Less";
            case RHICompareOp::Equal:
                return "Equal";
            case RHICompareOp::LessOrEqual:
                return "LessOrEqual";
            case RHICompareOp::Greater:
                return "Greater";
            case RHICompareOp::NotEqual:
                return "NotEqual";
            case RHICompareOp::GreaterOrEqual:
                return "GreaterOrEqual";
            case RHICompareOp::Always:
                return "Always";
        }

        return "LessOrEqual";
    }

    bool DrawCompareOpCombo(const char* label, RHICompareOp& compareOp)
    {
        static constexpr RHICompareOp compareOps[] = {RHICompareOp::Never,
                                                      RHICompareOp::Less,
                                                      RHICompareOp::Equal,
                                                      RHICompareOp::LessOrEqual,
                                                      RHICompareOp::Greater,
                                                      RHICompareOp::NotEqual,
                                                      RHICompareOp::GreaterOrEqual,
                                                      RHICompareOp::Always};

        bool changed = false;
        if (ImGui::BeginCombo(label, GetCompareOpName(compareOp)))
        {
            for (auto value : compareOps)
            {
                bool selected = value == compareOp;
                if (ImGui::Selectable(GetCompareOpName(value), selected))
                {
                    compareOp = value;
                    changed = true;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        return changed;
    }

    bool DrawImageUsageCheckbox(const char* label, RHIImageUsages& usages, RHIImageUsageFlagBits flag)
    {
        bool enabled = usages & flag;
        if (ImGui::Checkbox(label, &enabled))
        {
            if (enabled)
                usages |= flag;
            else
                usages &= ~flag;
            return true;
        }

        return false;
    }

    std::string GetAssetLabel(const std::vector<AssetRegistryTerm*>& assets, Hazel::UUID uuid)
    {
        if (uuid == Hazel::UUID(-1)) { return "None"; }

        auto it = std::ranges::find_if(assets, [uuid](const AssetRegistryTerm* asset) { return asset->uuid == uuid; });
        if (it == assets.end()) { return "None"; }

        return (*it)->filePath.filename().string();
    }

    bool DrawAssetRegistryCombo(const char* label,
                                const std::vector<AssetRegistryTerm*>& assets,
                                Hazel::UUID& value,
                                bool allowNone)
    {
        bool changed = false;
        std::string currentLabel = GetAssetLabel(assets, value);
        if (ImGui::BeginCombo(label, currentLabel.c_str()))
        {
            if (allowNone)
            {
                bool selected = value == Hazel::UUID(-1);
                if (ImGui::Selectable("None", selected) && value != Hazel::UUID(-1))
                {
                    value = Hazel::UUID(-1);
                    changed = true;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }

            for (const auto& asset : assets)
            {
                bool selected = value == asset->uuid;
                const auto itemLabel = asset->filePath.filename().string();
                if (ImGui::Selectable(itemLabel.c_str(), selected) && value != asset->uuid)
                {
                    value = asset->uuid;
                    changed = true;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        return changed;
    }
} // namespace Aster::PropertyPanelHelpers
