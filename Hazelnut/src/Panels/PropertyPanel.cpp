#include "PropertyPanel.h"

#include "Hazel/Asset/ComputeShaderAsset.h"
#include "Hazel/Asset/RenderTextureAsset.h"
#include "Hazel/Asset/SamplerAsset.h"
#include "Hazel/Asset/ShaderAsset.h"
#include "Hazel/Asset/TextureAsset.h"
#include "Hazel/Project/Project.h"
#include "Hazel/RHI/RHI.h"
#include "Hazel/Scene/Components.h"
#include "Hazel/Scripting/ScriptEngine.h"
#include "Hazel/UI/UI.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <yaml-cpp/yaml.h>

#include <glm/glm.hpp>

#include <fstream>
#include <unordered_set>

namespace Hazel
{
    namespace
    {
        void DrawVec3Control(const std::string& label,
                             glm::vec3& values,
                             float resetValue = 0.0f,
                             float columnWidth = 100.0f)
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
            ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
            ImGui::PushFont(boldFont);
            if (ImGui::Button("X", buttonSize))
                values.x = resetValue;
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
            if (ImGui::Button("Y", buttonSize))
                values.y = resetValue;
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
            if (ImGui::Button("Z", buttonSize))
                values.z = resetValue;
            ImGui::PopFont();
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
            ImGui::PopItemWidth();

            ImGui::PopStyleVar();
            ImGui::Columns(1);
            ImGui::PopID();
        }

        template <typename T, typename UIFunction>
        void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction)
        {
            const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                                     ImGuiTreeNodeFlags_SpanAvailWidth |
                                                     ImGuiTreeNodeFlags_AllowOverlap |
                                                     ImGuiTreeNodeFlags_FramePadding;
            if (entity.HasComponent<T>())
            {
                auto& component = entity.GetComponent<T>();
                ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
                float lineHeight = GImGui->Font->LegacySize + GImGui->Style.FramePadding.y * 2.0f;
                ImGui::Separator();
                bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
                ImGui::PopStyleVar();
                ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
                if (ImGui::Button("+", ImVec2{lineHeight, lineHeight}))
                    ImGui::OpenPopup("ComponentSettings");

                bool removeComponent = false;
                if (ImGui::BeginPopup("ComponentSettings"))
                {
                    if (ImGui::MenuItem("Remove component"))
                        removeComponent = true;

                    ImGui::EndPopup();
                }

                if (open)
                {
                    uiFunction(component);
                    ImGui::TreePop();
                }

                if (removeComponent)
                    entity.RemoveComponent<T>();
            }
        }

        void WriteMetaFile(const TextureAsset& asset)
        {
            std::ofstream output(asset.GetFilePath().string() + ".meta");
            output << asset.GetMeta().Serialize();
        }

        void WriteMetaFile(const ComputeShaderAsset& asset)
        {
            std::ofstream output(asset.GetFilePath().string() + ".meta");
            output << asset.GetMeta().Serialize();
        }

        void WriteMetaFile(const ShaderAsset& asset)
        {
            std::ofstream output(asset.GetFilePath().string() + ".meta");
            output << asset.GetMeta().Serialize();
        }

        void WriteMetaFile(const RenderTextureAsset& asset)
        {
            std::ofstream output(asset.GetFilePath().string());
            output << asset.GetMeta().Serialize();
        }

        void WriteMetaFile(const SamplerAsset& asset)
        {
            std::ofstream output(asset.GetFilePath().string());
            output << asset.GetMeta().Serialize();
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
            static const RHIFormat formats[] = {
                RHIFormat::Undefined, RHIFormat::R8UNorm, RHIFormat::R32SInt, RHIFormat::RG8UNorm,
                RHIFormat::R32SFloat, RHIFormat::RG32SFloat, RHIFormat::RGB32SFloat, RHIFormat::RG16UNorm,
                RHIFormat::BGRA8UNorm, RHIFormat::BGRA8SRGB, RHIFormat::RGBA8UNorm, RHIFormat::RGBA8SRGB,
                RHIFormat::RGB10A2UNorm, RHIFormat::RGBA16SFloat, RHIFormat::D32SFloat,
                RHIFormat::D32SFloatS8Uint, RHIFormat::S8Uint
            };

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
                    if (selected)
                        ImGui::SetItemDefaultFocus();
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
            static const RHIImageViewType viewTypes[] = {
                Image1D, Image2D, Image3D, Cube, Image1DArray, Image2DArray, CubeArray
            };

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
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            return changed;
        }

        const char* GetFilterName(RHISamplerFilter filter)
        {
            return filter == RHISamplerFilter::Nearest ? "Nearest" : "Linear";
        }

        bool DrawFilterCombo(const char* label, RHISamplerFilter& filter)
        {
            static const RHISamplerFilter filters[] = {RHISamplerFilter::Nearest, RHISamplerFilter::Linear};

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
                    if (selected)
                        ImGui::SetItemDefaultFocus();
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
            static const RHISamplerAddressMode modes[] = {
                RHISamplerAddressMode::Repeat,
                RHISamplerAddressMode::MirroredRepeat,
                RHISamplerAddressMode::ClampToEdge,
                RHISamplerAddressMode::ClampToBorder
            };

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
                    if (selected)
                        ImGui::SetItemDefaultFocus();
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
            static const RHICompareOp compareOps[] = {
                RHICompareOp::Never, RHICompareOp::Less, RHICompareOp::Equal, RHICompareOp::LessOrEqual,
                RHICompareOp::Greater, RHICompareOp::NotEqual, RHICompareOp::GreaterOrEqual, RHICompareOp::Always
            };

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
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            return changed;
        }

        bool DrawImageUsageCheckbox(const char* label, RHIImageUsages& usages, RHIImageUsageFlagBits flag)
        {
            bool enabled = static_cast<bool>(usages & flag);
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
    } // namespace

    void PropertyPanel::SetContext(const Ref<Scene>& scene)
    {
        m_Context = scene;
        m_SelectedEntity = {};
        if (m_SelectionType == SelectionType::Entity)
            m_SelectionType = SelectionType::None;
    }

    void PropertyPanel::SetSelectedEntity(Entity entity)
    {
        m_SelectedEntity = entity;
        m_SelectionType = entity ? SelectionType::Entity : SelectionType::None;
    }

    void PropertyPanel::SetSelectedMetaPath(const std::filesystem::path& metaPath)
    {
        m_SelectedMetaPath = metaPath;
        m_SelectionType = metaPath.empty() ? SelectionType::None : SelectionType::Meta;
    }

    void PropertyPanel::OnImGuiRender()
    {
        ImGui::Begin("Properties");

        if (m_SelectionType == SelectionType::Entity && m_SelectedEntity)
            DrawEntityProperties(m_SelectedEntity);
        else if (m_SelectionType == SelectionType::Meta && !m_SelectedMetaPath.empty())
            DrawAssetProperties();

        ImGui::End();
    }

    void PropertyPanel::DrawEntityProperties(Entity entity)
    {
        if (entity.HasComponent<TagComponent>())
        {
            auto& tag = entity.GetComponent<TagComponent>().tag;

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strncpy_s(buffer, sizeof(buffer), tag.c_str(), sizeof(buffer));
            if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
                tag = std::string(buffer);
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponent");

        if (ImGui::BeginPopup("AddComponent"))
        {
            DisplayAddComponentEntry<CameraComponent>("Camera");
            DisplayAddComponentEntry<MeshRendererComponent>("Mesh Renderer");
            DisplayAddComponentEntry<ScriptComponent>("Script");
            ImGui::EndPopup();
        }
        ImGui::PopItemWidth();

        DrawComponent<TransformComponent>("Transform",
                                          entity,
                                          [](auto& component) {
                                              DrawVec3Control("Translation", component.translation);
                                              glm::vec3 rotation = glm::degrees(component.rotation);
                                              DrawVec3Control("Rotation", rotation);
                                              component.rotation = glm::radians(rotation);
                                              DrawVec3Control("Scale", component.scale, 1.0f);
                                          });

        DrawComponent<CameraComponent>("Camera",
                                       entity,
                                       [](auto& component) {
                                           auto& camera = component.camera;
                                           ImGui::Checkbox("Primary", &component.isPrimary);

                                           const char* projectionTypeStrings[] = {"Perspective", "Orthographic"};
                                           const char* currentProjectionTypeString = projectionTypeStrings[static_cast<
                                               int>(camera.GetProjectionType())];
                                           if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
                                           {
                                               for (int i = 0; i < 2; i++)
                                               {
                                                   bool isSelected =
                                                       currentProjectionTypeString == projectionTypeStrings[i];
                                                   if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
                                                   {
                                                       currentProjectionTypeString = projectionTypeStrings[i];
                                                       camera.SetProjectionType(static_cast<Camera::ProjectionType>(i));
                                                   }

                                                   if (isSelected)
                                                       ImGui::SetItemDefaultFocus();
                                               }
                                               ImGui::EndCombo();
                                           }

                                           if (camera.GetProjectionType() == Camera::ProjectionType::Perspective)
                                           {
                                               float perspectiveVerticalFov = glm::degrees(camera.GetFovX());
                                               if (ImGui::DragFloat("Vertical FOV", &perspectiveVerticalFov))
                                                   camera.SetFovX(glm::radians(perspectiveVerticalFov));

                                               float perspectiveNear = camera.GetNearClip();
                                               if (ImGui::DragFloat("Near", &perspectiveNear))
                                                   camera.SetNearClip(perspectiveNear);

                                               float perspectiveFar = camera.GetFarClip();
                                               if (ImGui::DragFloat("Far", &perspectiveFar))
                                                   camera.SetFarClip(perspectiveFar);
                                           }

                                           if (camera.GetProjectionType() == Camera::ProjectionType::Orthographic)
                                           {
                                               float orthoWidth = camera.GetOrthoWidth();
                                               if (ImGui::DragFloat("Width", &orthoWidth))
                                                   camera.SetOrthoWidth(orthoWidth);

                                               float orthoNear = camera.GetOrthoNearClip();
                                               if (ImGui::DragFloat("Near", &orthoNear))
                                                   camera.SetOrthoNearClip(orthoNear);

                                               float orthoFar = camera.GetOrthoFarClip();
                                               if (ImGui::DragFloat("Far", &orthoFar))
                                                   camera.SetOrthoFarClip(orthoFar);
                                           }
                                       });

        DrawComponent<ScriptComponent>("Script",
                                       entity,
                                       [entity, scene = m_Context](auto& component) mutable {
                                           bool scriptClassExists =
                                               ScriptEngine::EntityClassExists(component.className);

                                           static char buffer[64];
                                           strcpy_s(buffer, sizeof(buffer), component.className.c_str());

                                           UI::ScopedStyleColor textColor(
                                               ImGuiCol_Text,
                                               ImVec4(0.9f, 0.2f, 0.3f, 1.0f),
                                               !scriptClassExists);

                                           if (ImGui::InputText("Class", buffer, sizeof(buffer)))
                                           {
                                               component.className = buffer;
                                               return;
                                           }

                                           if (scene->IsRunning())
                                           {
                                               Ref<ScriptInstance> scriptInstance =
                                                   ScriptEngine::GetEntityScriptInstance(entity.GetUUID());
                                               if (scriptInstance)
                                               {
                                                   const auto& fields = scriptInstance->GetScriptClass()->GetFields();
                                                   for (const auto& [name, field] : fields)
                                                   {
                                                       if (field.Type == ScriptFieldType::Float)
                                                       {
                                                           float data = scriptInstance->GetFieldValue<float>(name);
                                                           if (ImGui::DragFloat(name.c_str(), &data))
                                                               scriptInstance->SetFieldValue(name, data);
                                                       }
                                                   }
                                               }
                                           }
                                           else if (scriptClassExists)
                                           {
                                               Ref<ScriptClass> entityClass = ScriptEngine::GetEntityClass(
                                                   component.className);
                                               const auto& fields = entityClass->GetFields();

                                               auto& entityFields = ScriptEngine::GetScriptFieldMap(entity);
                                               for (const auto& [name, field] : fields)
                                               {
                                                   if (entityFields.find(name) != entityFields.end())
                                                   {
                                                       ScriptFieldInstance& scriptField = entityFields.at(name);
                                                       if (field.Type == ScriptFieldType::Float)
                                                       {
                                                           float data = scriptField.GetValue<float>();
                                                           if (ImGui::DragFloat(name.c_str(), &data))
                                                               scriptField.SetValue(data);
                                                       }
                                                   }
                                                   else if (field.Type == ScriptFieldType::Float)
                                                   {
                                                       float data = 0.0f;
                                                       if (ImGui::DragFloat(name.c_str(), &data))
                                                       {
                                                           ScriptFieldInstance& fieldInstance = entityFields[name];
                                                           fieldInstance.Field = field;
                                                           fieldInstance.SetValue(data);
                                                       }
                                                   }
                                               }
                                           }
                                       });

        DrawComponent<MeshRendererComponent>("Mesh Renderer",
                                             entity,
                                             [](auto& component) {
                                                 ImGui::Text("TODO");
                                             });
    }

    void PropertyPanel::DrawAssetProperties()
    {
        ImGui::TextUnformatted(m_SelectedMetaPath.filename().string().c_str());
        ImGui::TextWrapped("%s", m_SelectedMetaPath.string().c_str());
        ImGui::Separator();

        YAML::Node metaNode = YAML::LoadFile(m_SelectedMetaPath.string());
        if (!metaNode["UUID"])
            return;

        UUID uuid = UUID(metaNode["UUID"].as<uint64_t>());
        auto& assetManager = Project::GetActive()->GetAssetManager();
        auto assetType = assetManager.GetAssetType(uuid);

        if (assetType == AssetType::Texture)
        {
            auto* asset = assetManager.GetAsset<TextureAsset>(uuid);
            if (!asset)
                return;

            auto& meta = asset->GetMeta();
            bool changed = false;
            changed |= ImGui::Checkbox("sRGB", &meta.isSRGB);
            changed |= ImGui::Checkbox("Use Mipmap", &meta.useMipmap);
            if (changed)
            {
                WriteMetaFile(*asset);
                asset->Recreate();
            }
            return;
        }

        if (assetType == AssetType::ComputeShader)
        {
            auto* asset = assetManager.GetAsset<ComputeShaderAsset>(uuid);
            if (!asset)
                return;

            ImGui::Text("Type: Compute Shader");
            ImGui::Text("UUID: %llu", static_cast<uint64_t>(asset->GetUUID()));
            ImGui::TextWrapped("Source: %s", asset->GetFilePath().string().c_str());
            return;
        }

        if (assetType == AssetType::Shader)
        {
            auto* asset = assetManager.GetAsset<ShaderAsset>(uuid);
            if (!asset)
                return;

            ImGui::Text("Type: Shader");
            ImGui::Text("UUID: %llu", static_cast<uint64_t>(asset->GetUUID()));
            ImGui::TextWrapped("Source: %s", asset->GetFilePath().string().c_str());
            return;
        }

        if (assetType == AssetType::RenderTexture)
        {
            auto* asset = assetManager.GetAsset<RenderTextureAsset>(uuid);
            if (!asset)
                return;

            auto& desc = asset->GetMeta().desc;
            bool changed = false;
            changed |= ImGui::DragScalar("Width", ImGuiDataType_U32, &desc.width, 1.0f);
            changed |= ImGui::DragScalar("Height", ImGuiDataType_U32, &desc.height, 1.0f);
            changed |= ImGui::DragScalar("Depth", ImGuiDataType_U32, &desc.depth, 1.0f);
            changed |= ImGui::DragScalar("Array Layers", ImGuiDataType_U32, &desc.arrayLayers, 1.0f);
            changed |= DrawViewTypeCombo("View Type", desc.viewType);
            changed |= ImGui::Checkbox("Use Mipmap", &desc.useMipmap);
            changed |= ImGui::Checkbox("Per Frame", &desc.perFrame);
            changed |= DrawFormatCombo("Format", desc.format);
            changed |= DrawImageUsageCheckbox("Transfer Source", desc.usages, RHIImageUsageFlagBits::TransferSource);
            changed |= DrawImageUsageCheckbox("Transfer Destination",
                                              desc.usages,
                                              RHIImageUsageFlagBits::TransferDestination);
            changed |= DrawImageUsageCheckbox("Sampled", desc.usages, RHIImageUsageFlagBits::Sampled);
            changed |= DrawImageUsageCheckbox("Storage", desc.usages, RHIImageUsageFlagBits::Storage);
            changed |= DrawImageUsageCheckbox("Color Attachment", desc.usages, RHIImageUsageFlagBits::ColorAttachment);
            changed |= DrawImageUsageCheckbox("Depth Stencil Attachment",
                                              desc.usages,
                                              RHIImageUsageFlagBits::DepthStencilAttachment);
            if (changed)
            {
                WriteMetaFile(*asset);
                asset->Recreate();
            }
            return;
        }

        if (assetType == AssetType::Sampler)
        {
            auto* asset = assetManager.GetAsset<SamplerAsset>(uuid);
            if (!asset)
                return;

            auto& desc = asset->GetMeta().desc;
            bool changed = false;
            changed |= DrawFilterCombo("Min Filter", desc.minFilter);
            changed |= DrawFilterCombo("Mag Filter", desc.magFilter);
            changed |= DrawFilterCombo("Mip Filter", desc.mipFilter);
            changed |= DrawAddressModeCombo("Address Mode U", desc.addressModeU);
            changed |= DrawAddressModeCombo("Address Mode V", desc.addressModeV);
            changed |= DrawAddressModeCombo("Address Mode W", desc.addressModeW);
            changed |= ImGui::DragFloat("Mip LOD Bias", &desc.mipLodBias);
            changed |= ImGui::DragFloat("Min LOD", &desc.minLod);
            changed |= ImGui::DragFloat("Max LOD", &desc.maxLod);
            changed |= ImGui::DragFloat("Max Anisotropy", &desc.maxAnisotropy);
            changed |= ImGui::Checkbox("Enable Anisotropy", &desc.enableAnisotropy);
            changed |= ImGui::Checkbox("Compare Enable", &desc.compareEnable);
            changed |= DrawCompareOpCombo("Compare Op", desc.compareOp);
            if (changed)
            {
                WriteMetaFile(*asset);
                asset->Recreate();
            }
        }
    }

    template <typename T>
    void PropertyPanel::DisplayAddComponentEntry(const std::string& entryName)
    {
        if (!m_SelectedEntity.HasComponent<T>())
        {
            if (ImGui::MenuItem(entryName.c_str()))
            {
                m_SelectedEntity.AddComponent<T>();
                ImGui::CloseCurrentPopup();
            }
        }
    }
}