#include "PropertyPanel.h"

#include "Hazel/Asset/AssetUtils.h"
#include "Hazel/Asset/ComputeShaderAsset.h"
#include "Hazel/Asset/MaterialAsset.h"
#include "Hazel/Asset/MeshAsset.h"
#include "Hazel/Asset/RenderTextureAsset.h"
#include "Hazel/Asset/SamplerAsset.h"
#include "Hazel/Asset/ShaderAsset.h"
#include "Hazel/Asset/TextureAsset.h"
#include "Hazel/Project/Project.h"
#include "Hazel/RHI/RHI.h"
#include "Hazel/Scene/Components.h"
#include "Hazel/Scripting/ScriptEngine.h"
#include "Hazel/UI/UI.h"
#include "PropertyPanelHelpers.h"

#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui_internal.h>
#include <unordered_set>
#include <yaml-cpp/yaml.h>

namespace Aster
{
    namespace
    {
        template <typename T, typename UIFunction>
        void DrawComponent(const std::string& name, Hazel::Entity entity, UIFunction uiFunction)
        {
            constexpr ImGuiTreeNodeFlags treeNodeFlags =
                ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth
                | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;
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
                if (ImGui::Button("+", ImVec2{lineHeight, lineHeight})) ImGui::OpenPopup("ComponentSettings");

                bool removeComponent = false;
                if (ImGui::BeginPopup("ComponentSettings"))
                {
                    if (ImGui::MenuItem("Remove component")) removeComponent = true;

                    ImGui::EndPopup();
                }

                if (open)
                {
                    uiFunction(component);
                    ImGui::TreePop();
                }

                if (removeComponent) entity.RemoveComponent<T>();
            }
        }
    } // namespace

    void PropertyPanel::SetContext(const Hazel::Ref<Hazel::Scene>& scene)
    {
        m_Context = scene;
        m_SelectedEntity = {};
        if (m_SelectionType == SelectionType::Entity) m_SelectionType = SelectionType::None;
    }

    void PropertyPanel::SetSelectedEntity(Hazel::Entity entity)
    {
        m_SelectedEntity = entity;
        m_SelectionType = entity ? SelectionType::Entity : SelectionType::None;
    }

    void PropertyPanel::SetSelectedMetaPath(const std::filesystem::path& metaPath)
    {
        m_SelectedMetaPath = metaPath;
        if (std::filesystem::exists(metaPath) && std::filesystem::is_regular_file(metaPath)
            && metaPath.extension() == ".meta")
        {
            m_SelectionType = SelectionType::Meta;
        }
        else
        {
            m_SelectionType = SelectionType::None;
        }
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

    void PropertyPanel::DrawEntityProperties(Hazel::Entity entity)
    {
        if (entity.HasComponent<Hazel::TagComponent>())
        {
            auto& tag = entity.GetComponent<Hazel::TagComponent>().tag;

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strncpy_s(buffer, sizeof(buffer), tag.c_str(), sizeof(buffer));
            if (ImGui::InputText("##Tag", buffer, sizeof(buffer))) tag = std::string(buffer);
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        if (ImGui::Button("Add Component")) ImGui::OpenPopup("AddComponent");

        if (ImGui::BeginPopup("AddComponent"))
        {
            DisplayAddComponentEntry<Hazel::CameraComponent>("Camera");
            DisplayAddComponentEntry<Hazel::MeshRendererComponent>("Mesh Renderer");
            DisplayAddComponentEntry<Hazel::ScriptComponent>("Script");
            ImGui::EndPopup();
        }
        ImGui::PopItemWidth();

        DrawComponent<Hazel::TransformComponent>("Transform", entity, [entity](auto& component) mutable {
            glm::vec3 translation = component.translation;
            PropertyPanelHelpers::DrawVec3Control("Translation", translation);
            if (glm::any(glm::notEqual(translation, component.translation))) { entity.SetTranslation(translation); }

            glm::vec3 rotation = glm::degrees(component.rotation);
            PropertyPanelHelpers::DrawVec3Control("Rotation", rotation);
            const glm::vec3 rotationRadians = glm::radians(rotation);
            if (glm::any(glm::notEqual(rotationRadians, component.rotation))) { entity.SetRotation(rotationRadians); }

            glm::vec3 scale = component.scale;
            PropertyPanelHelpers::DrawVec3Control("Scale", scale, 1.0f);
            if (glm::any(glm::notEqual(scale, component.scale))) { entity.SetScale(scale); }
        });

        DrawComponent<Hazel::CameraComponent>("Camera", entity, [](auto& component) {
            auto& camera = component.camera;
            auto* assetManager = Hazel::Project::GetActive()->GetAssetManager();
            auto renderTextures = assetManager->GetAssetsByType(AssetType::RenderTexture);
            ImGui::Checkbox("Primary", &component.isPrimary);

            const char* projectionTypeStrings[] = {"Perspective", "Orthographic"};
            const char* currentProjectionTypeString =
                projectionTypeStrings[static_cast<int>(camera.GetProjectionType())];
            if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
            {
                for (int i = 0; i < 2; i++)
                {
                    bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
                    if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
                    {
                        currentProjectionTypeString = projectionTypeStrings[i];
                        camera.SetProjectionType(static_cast<Hazel::Camera::ProjectionType>(i));
                    }

                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (camera.GetProjectionType() == Hazel::Camera::ProjectionType::Perspective)
            {
                float perspectiveVerticalFov = glm::degrees(camera.GetFovX());
                if (ImGui::DragFloat("Vertical FOV", &perspectiveVerticalFov))
                    camera.SetFovX(glm::radians(perspectiveVerticalFov));

                float perspectiveNear = camera.GetNearClip();
                if (ImGui::DragFloat("Near", &perspectiveNear)) camera.SetNearClip(perspectiveNear);

                float perspectiveFar = camera.GetFarClip();
                if (ImGui::DragFloat("Far", &perspectiveFar)) camera.SetFarClip(perspectiveFar);
            }

            if (camera.GetProjectionType() == Hazel::Camera::ProjectionType::Orthographic)
            {
                float orthoWidth = camera.GetOrthoWidth();
                if (ImGui::DragFloat("Width", &orthoWidth)) camera.SetOrthoWidth(orthoWidth);

                float orthoNear = camera.GetOrthoNearClip();
                if (ImGui::DragFloat("Near", &orthoNear)) camera.SetOrthoNearClip(orthoNear);

                float orthoFar = camera.GetOrthoFarClip();
                if (ImGui::DragFloat("Far", &orthoFar)) camera.SetOrthoFarClip(orthoFar);
            }

            PropertyPanelHelpers::DrawAssetRegistryCombo("Render Texture", renderTextures, component.renderTextureUUID);
        });

        DrawComponent<Hazel::ScriptComponent>("Script", entity, [entity, scene = m_Context](auto& component) mutable {
            bool scriptClassExists = Hazel::ScriptEngine::EntityClassExists(component.className);

            static char buffer[64];
            strcpy_s(buffer, sizeof(buffer), component.className.c_str());

            Hazel::UI::ScopedStyleColor textColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.3f, 1.0f), !scriptClassExists);

            if (ImGui::InputText("Class", buffer, sizeof(buffer)))
            {
                component.className = buffer;
                return;
            }

            if (scene->IsRunning())
            {
                Hazel::Ref<Hazel::ScriptInstance> scriptInstance =
                    Hazel::ScriptEngine::GetEntityScriptInstance(entity.GetUUID());
                if (scriptInstance)
                {
                    const auto& fields = scriptInstance->GetScriptClass()->GetFields();
                    for (const auto& [name, field] : fields)
                    {
                        if (field.Type == Hazel::ScriptFieldType::Float)
                        {
                            float data = scriptInstance->GetFieldValue<float>(name);
                            if (ImGui::DragFloat(name.c_str(), &data)) scriptInstance->SetFieldValue(name, data);
                        }
                    }
                }
            }
            else if (scriptClassExists)
            {
                Hazel::Ref<Hazel::ScriptClass> entityClass = Hazel::ScriptEngine::GetEntityClass(component.className);
                const auto& fields = entityClass->GetFields();

                auto& entityFields = Hazel::ScriptEngine::GetScriptFieldMap(entity);
                for (const auto& [name, field] : fields)
                {
                    if (entityFields.contains(name))
                    {
                        Hazel::ScriptFieldInstance& scriptField = entityFields.at(name);
                        if (field.Type == Hazel::ScriptFieldType::Float)
                        {
                            float data = scriptField.GetValue<float>();
                            if (ImGui::DragFloat(name.c_str(), &data)) scriptField.SetValue(data);
                        }
                    }
                    else if (field.Type == Hazel::ScriptFieldType::Float)
                    {
                        float data = 0.0f;
                        if (ImGui::DragFloat(name.c_str(), &data))
                        {
                            Hazel::ScriptFieldInstance& fieldInstance = entityFields[name];
                            fieldInstance.Field = field;
                            fieldInstance.SetValue(data);
                        }
                    }
                }
            }
        });

        DrawComponent<Hazel::MeshRendererComponent>("Mesh Renderer", entity, [entity](auto& component) mutable {
            auto* assetManager = Hazel::Project::GetActive()->GetAssetManager();
            auto meshes = assetManager->GetAssetsByType(AssetType::Mesh);
            auto materials = assetManager->GetAssetsByType(AssetType::Material);

            Hazel::UUID meshUUID = component.meshUUID;
            if (PropertyPanelHelpers::DrawAssetRegistryCombo("Mesh", meshes, meshUUID)) { entity.SetMesh(meshUUID); }

            Hazel::UUID materialUUID = component.materialUUID;
            if (PropertyPanelHelpers::DrawAssetRegistryCombo("Material", materials, materialUUID))
            {
                entity.SetMaterial(materialUUID);
            }
        });
    }

    void PropertyPanel::DrawAssetProperties()
    {
        ImGui::TextUnformatted(m_SelectedMetaPath.filename().string().c_str());
        ImGui::TextWrapped("%s", m_SelectedMetaPath.string().c_str());
        ImGui::Separator();

        YAML::Node metaNode = YAML::LoadFile(m_SelectedMetaPath.string());
        if (!metaNode["UUID"]) return;

        auto uuid = Hazel::UUID(metaNode["UUID"].as<uint64_t>());
        auto* assetManager = Hazel::Project::GetActive()->GetAssetManager();
        auto assetType = assetManager->GetAssetType(uuid);

        if (assetType == AssetType::Texture)
        {
            auto* asset = static_cast<TextureAsset*>(assetManager->RequestAssetBlocked(uuid));
            if (!asset) return;

            auto& meta = asset->GetMeta();
            bool changed = false;
            bool isSRGB = meta.IsSRGB();
            if (ImGui::Checkbox("sRGB", &isSRGB))
            {
                meta.SetSRGB(isSRGB);
                changed = true;
            }
            bool useMipmap = meta.UseMipmap();
            if (ImGui::Checkbox("Use Mipmap", &useMipmap))
            {
                meta.SetUseMipmap(useMipmap);
                changed = true;
            }
            if (changed) { WriteMetaToFile(asset); }
            return;
        }

        if (assetType == AssetType::ComputeShader)
        {
            auto* asset = static_cast<ComputeShaderAsset*>(assetManager->RequestAssetBlocked(uuid));
            if (!asset) return;

            ImGui::Text("Type: Compute Shader");
            ImGui::Text("UUID: %llu", static_cast<uint64_t>(asset->GetUUID()));
            ImGui::TextWrapped("Source: %s", asset->GetFilePath().string().c_str());
            return;
        }

        if (assetType == AssetType::Shader)
        {
            auto* asset = static_cast<ShaderAsset*>(assetManager->RequestAssetBlocked(uuid));
            if (!asset) return;

            ImGui::Text("Type: Shader");
            ImGui::Text("UUID: %llu", static_cast<uint64_t>(asset->GetUUID()));
            ImGui::TextWrapped("Source: %s", asset->GetFilePath().string().c_str());
            return;
        }

        if (assetType == AssetType::RenderTexture)
        {
            auto* asset = static_cast<RenderTextureAsset*>(assetManager->RequestAssetBlocked(uuid));
            if (!asset) return;

            auto& meta = asset->GetMeta();
            const auto& desc = meta.GetDesc();
            bool changed = false;
            uint32_t width = desc.width;
            if (ImGui::DragScalar("Width", ImGuiDataType_U32, &width, 1.0f))
            {
                meta.SetWidth(width);
                changed = true;
            }
            uint32_t height = desc.height;
            if (ImGui::DragScalar("Height", ImGuiDataType_U32, &height, 1.0f))
            {
                meta.SetHeight(height);
                changed = true;
            }
            uint32_t depth = desc.depth;
            if (ImGui::DragScalar("Depth", ImGuiDataType_U32, &depth, 1.0f))
            {
                meta.SetDepth(depth);
                changed = true;
            }
            uint32_t arrayLayers = desc.arrayLayers;
            if (ImGui::DragScalar("Array Layers", ImGuiDataType_U32, &arrayLayers, 1.0f))
            {
                meta.SetArrayLayers(arrayLayers);
                changed = true;
            }
            auto viewType = desc.viewType;
            if (PropertyPanelHelpers::DrawViewTypeCombo("View Type", viewType))
            {
                meta.SetViewType(viewType);
                changed = true;
            }
            bool useMipmap = desc.useMipmap;
            if (ImGui::Checkbox("Use Mipmap", &useMipmap))
            {
                meta.SetUseMipmap(useMipmap);
                changed = true;
            }
            bool perFrame = desc.perFrame;
            if (ImGui::Checkbox("Per Frame", &perFrame))
            {
                meta.SetPerFrame(perFrame);
                changed = true;
            }
            auto format = desc.format;
            if (PropertyPanelHelpers::DrawFormatCombo("Format", format))
            {
                meta.SetFormat(format);
                changed = true;
            }
            auto usages = desc.usages;
            changed |= PropertyPanelHelpers::DrawImageUsageCheckbox(
                "Transfer Source", usages, RHIImageUsageFlagBits::TransferSource);
            changed |= PropertyPanelHelpers::DrawImageUsageCheckbox(
                "Transfer Destination", usages, RHIImageUsageFlagBits::TransferDestination);
            changed |= PropertyPanelHelpers::DrawImageUsageCheckbox("Sampled", usages, RHIImageUsageFlagBits::Sampled);
            changed |= PropertyPanelHelpers::DrawImageUsageCheckbox("Storage", usages, RHIImageUsageFlagBits::Storage);
            changed |= PropertyPanelHelpers::DrawImageUsageCheckbox(
                "Color Attachment", usages, RHIImageUsageFlagBits::ColorAttachment);
            changed |= PropertyPanelHelpers::DrawImageUsageCheckbox(
                "Depth Stencil Attachment", usages, RHIImageUsageFlagBits::DepthStencilAttachment);
            meta.SetUsages(usages);
            if (changed) { WriteMetaToFile(asset); }
            return;
        }

        if (assetType == AssetType::Sampler)
        {
            auto* asset = static_cast<SamplerAsset*>(assetManager->RequestAssetBlocked(uuid));
            if (!asset) return;

            auto& meta = asset->GetMeta();
            const auto& desc = meta.GetDesc();
            bool changed = false;
            auto minFilter = desc.minFilter;
            if (PropertyPanelHelpers::DrawFilterCombo("Min Filter", minFilter))
            {
                meta.SetMinFilter(minFilter);
                changed = true;
            }
            auto magFilter = desc.magFilter;
            if (PropertyPanelHelpers::DrawFilterCombo("Mag Filter", magFilter))
            {
                meta.SetMagFilter(magFilter);
                changed = true;
            }
            auto mipFilter = desc.mipFilter;
            if (PropertyPanelHelpers::DrawFilterCombo("Mip Filter", mipFilter))
            {
                meta.SetMipFilter(mipFilter);
                changed = true;
            }
            auto addressModeU = desc.addressModeU;
            if (PropertyPanelHelpers::DrawAddressModeCombo("Address Mode U", addressModeU))
            {
                meta.SetAddressModeU(addressModeU);
                changed = true;
            }
            auto addressModeV = desc.addressModeV;
            if (PropertyPanelHelpers::DrawAddressModeCombo("Address Mode V", addressModeV))
            {
                meta.SetAddressModeV(addressModeV);
                changed = true;
            }
            auto addressModeW = desc.addressModeW;
            if (PropertyPanelHelpers::DrawAddressModeCombo("Address Mode W", addressModeW))
            {
                meta.SetAddressModeW(addressModeW);
                changed = true;
            }
            float mipLodBias = desc.mipLodBias;
            if (ImGui::DragFloat("Mip LOD Bias", &mipLodBias))
            {
                meta.SetMipLodBias(mipLodBias);
                changed = true;
            }
            float minLod = desc.minLod;
            if (ImGui::DragFloat("Min LOD", &minLod))
            {
                meta.SetMinLod(minLod);
                changed = true;
            }
            float maxLod = desc.maxLod;
            if (ImGui::DragFloat("Max LOD", &maxLod))
            {
                meta.SetMaxLod(maxLod);
                changed = true;
            }
            float maxAnisotropy = desc.maxAnisotropy;
            if (ImGui::DragFloat("Max Anisotropy", &maxAnisotropy))
            {
                meta.SetMaxAnisotropy(maxAnisotropy);
                changed = true;
            }
            bool enableAnisotropy = desc.enableAnisotropy;
            if (ImGui::Checkbox("Enable Anisotropy", &enableAnisotropy))
            {
                meta.SetEnableAnisotropy(enableAnisotropy);
                changed = true;
            }
            bool compareEnable = desc.compareEnable;
            if (ImGui::Checkbox("Compare Enable", &compareEnable))
            {
                meta.SetCompareEnable(compareEnable);
                changed = true;
            }
            auto compareOp = desc.compareOp;
            if (PropertyPanelHelpers::DrawCompareOpCombo("Compare Op", compareOp))
            {
                meta.SetCompareOp(compareOp);
                changed = true;
            }
            if (changed) { WriteMetaToFile(asset); }
        }

        if (assetType == AssetType::Mesh)
        {
            auto* asset = static_cast<MeshAsset*>(assetManager->RequestAssetBlocked(uuid));
            if (!asset) return;

            ImGui::Text("Type: Mesh");
            ImGui::Text("UUID: %llu", static_cast<uint64_t>(asset->GetUUID()));
            ImGui::TextWrapped("Source: %s", asset->GetFilePath().string().c_str());
            bool changed = false;
            bool generateMeshlets = asset->GetMeta().GenerateMeshlets();
            if (ImGui::Checkbox("Generate Meshlets", &generateMeshlets))
            {
                asset->GetMeta().SetGenerateMeshlets(generateMeshlets);
                changed = true;
            }
            if (changed) { WriteMetaToFile(asset); }
            return;
        }

        if (assetType == AssetType::Material)
        {
            auto* asset = static_cast<MaterialAsset*>(assetManager->RequestAssetBlocked(uuid));
            if (!asset) return;
            auto shaders = assetManager->GetAssetsByType(AssetType::Shader);
            auto samplers = assetManager->GetAssetsByType(AssetType::Sampler);
            auto textures = assetManager->GetAssetsByType(AssetType::Texture);

            auto& meta = asset->GetMeta();

            ImGui::Text("Type: Material");
            ImGui::Text("UUID: %llu", static_cast<uint64_t>(asset->GetMeta().GetUUID()));
            ImGui::TextWrapped("Source: %s", asset->GetFilePath().string().c_str());

            bool changed = false;
            Hazel::UUID shaderUUID = meta.GetShader();
            if (PropertyPanelHelpers::DrawAssetRegistryCombo("Shader", shaders, shaderUUID))
            {
                meta.SetShader(shaderUUID);
                changed = true;
            }

            if (changed)
            {
                meta.RefreshShader(assetManager);
                WriteMetaToFile(asset);
            }

            auto pipelineState = meta.GetPipelineState();
            changed = false;
            changed |= PropertyPanelHelpers::DrawPolygonModeCombo("Polygon Mode", pipelineState.polygonMode);
            changed |= PropertyPanelHelpers::DrawCullModeCombo("Cull Mode", pipelineState.cullMode);
            changed |= ImGui::Checkbox("Depth Clamp", &pipelineState.depthClampEnable);
            changed |= ImGui::Checkbox("Depth Bias", &pipelineState.depthBiasEnable);
            changed |= ImGui::Checkbox("Depth Test", &pipelineState.depthTestEnable);
            changed |= ImGui::Checkbox("Depth Write", &pipelineState.depthWriteEnable);
            changed |= PropertyPanelHelpers::DrawCompareOpCombo("Depth Compare Op", pipelineState.depthCompareOp);
            changed |= ImGui::Checkbox("Stencil Test", &pipelineState.stencilTestEnable);
            if (changed)
            {
                meta.SetPipelineState(pipelineState);
                WriteMetaToFile(asset);
            }

            const auto& properties = meta.GetProperties();
            for (size_t propertyIndex = 0; propertyIndex < properties.size(); propertyIndex++)
            {
                const auto& property = properties[propertyIndex];
                bool propertyChanged = false;

                switch (property.type)
                {
                    case MaterialAssetPropertyType::Int:
                        {
                            int value;
                            std::memcpy(&value, property.data, sizeof(value));
                            if (ImGui::DragInt(property.name.c_str(), &value))
                            {
                                meta.SetPropertyData(propertyIndex, &value, sizeof(value));
                                propertyChanged = true;
                            }
                            break;
                        }
                    case MaterialAssetPropertyType::UInt:
                        {
                            uint32_t value;
                            std::memcpy(&value, property.data, sizeof(value));
                            if (ImGui::DragScalar(property.name.c_str(), ImGuiDataType_U32, &value))
                            {
                                meta.SetPropertyData(propertyIndex, &value, sizeof(value));
                                propertyChanged = true;
                            }
                            break;
                        }
                    case MaterialAssetPropertyType::Float:
                        {
                            float value;
                            std::memcpy(&value, property.data, sizeof(value));
                            if (ImGui::DragFloat(property.name.c_str(), &value))
                            {
                                meta.SetPropertyData(propertyIndex, &value, sizeof(value));
                                propertyChanged = true;
                            }
                            break;
                        }
                    case MaterialAssetPropertyType::Vec2:
                        {
                            float value[2];
                            std::memcpy(value, property.data, sizeof(value));
                            if (ImGui::DragFloat2(property.name.c_str(), value))
                            {
                                meta.SetPropertyData(propertyIndex, value, sizeof(value));
                                propertyChanged = true;
                            }
                            break;
                        }
                    case MaterialAssetPropertyType::Vec3:
                        {
                            float value[3];
                            std::memcpy(value, property.data, sizeof(value));
                            if (ImGui::DragFloat3(property.name.c_str(), value))
                            {
                                meta.SetPropertyData(propertyIndex, value, sizeof(value));
                                propertyChanged = true;
                            }
                            break;
                        }
                    case MaterialAssetPropertyType::Vec4:
                        {
                            float value[4];
                            std::memcpy(value, property.data, sizeof(value));
                            if (ImGui::DragFloat4(property.name.c_str(), value))
                            {
                                meta.SetPropertyData(propertyIndex, value, sizeof(value));
                                propertyChanged = true;
                            }
                            break;
                        }
                    case MaterialAssetPropertyType::Mat3:
                        {
                            float value[9];
                            std::memcpy(value, property.data, sizeof(value));
                            propertyChanged |= ImGui::DragFloat3((property.name + " 0").c_str(), value + 0);
                            propertyChanged |= ImGui::DragFloat3((property.name + " 1").c_str(), value + 3);
                            propertyChanged |= ImGui::DragFloat3((property.name + " 2").c_str(), value + 6);
                            if (propertyChanged) { meta.SetPropertyData(propertyIndex, value, sizeof(value)); }
                            break;
                        }
                    case MaterialAssetPropertyType::Mat4:
                        {
                            float value[16];
                            std::memcpy(value, property.data, sizeof(value));
                            propertyChanged |= ImGui::DragFloat4((property.name + " 0").c_str(), value + 0);
                            propertyChanged |= ImGui::DragFloat4((property.name + " 1").c_str(), value + 4);
                            propertyChanged |= ImGui::DragFloat4((property.name + " 2").c_str(), value + 8);
                            propertyChanged |= ImGui::DragFloat4((property.name + " 3").c_str(), value + 12);
                            if (propertyChanged) { meta.SetPropertyData(propertyIndex, value, sizeof(value)); }
                            break;
                        }
                    case MaterialAssetPropertyType::Sampler:
                        {
                            Hazel::UUID samplerUUID = property.sampler;
                            if (PropertyPanelHelpers::DrawAssetRegistryCombo(
                                    property.name.c_str(), samplers, samplerUUID))
                            {
                                meta.SetPropertySampler(propertyIndex, samplerUUID);
                                propertyChanged = true;
                            }
                            break;
                        }
                    case MaterialAssetPropertyType::Texture:
                        {
                            Hazel::UUID textureUUID = property.texture;
                            if (PropertyPanelHelpers::DrawAssetRegistryCombo(
                                    property.name.c_str(), textures, textureUUID))
                            {
                                meta.SetPropertyTexture(propertyIndex, textureUUID);
                                propertyChanged = true;
                            }
                            break;
                        }
                    case MaterialAssetPropertyType::SamplerWithTexture:
                        {
                            const auto samplerName = property.name + " Sampler";
                            const auto textureName = property.name + " Texture";
                            Hazel::UUID samplerUUID = property.sampler;
                            Hazel::UUID textureUUID = property.texture;

                            if (PropertyPanelHelpers::DrawAssetRegistryCombo(
                                    samplerName.c_str(), samplers, samplerUUID))
                            {
                                meta.SetPropertySampler(propertyIndex, samplerUUID);
                                propertyChanged = true;
                            }

                            if (PropertyPanelHelpers::DrawAssetRegistryCombo(
                                    textureName.c_str(), textures, textureUUID))
                            {
                                meta.SetPropertyTexture(propertyIndex, textureUUID);
                                propertyChanged = true;
                            }
                            break;
                        }
                }

                if (propertyChanged) { WriteMetaToFile(asset); }
            }
        }
    }

    template <typename T> void PropertyPanel::DisplayAddComponentEntry(const std::string& entryName)
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
} // namespace Aster
