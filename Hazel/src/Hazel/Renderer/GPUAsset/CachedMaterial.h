//
// Created by helmholtz on 2026/4/1.
//

#pragma once
#include "Hazel/Asset/MaterialAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUAsset.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/Renderer/Renderer.h"

#include <string>
#include <unordered_map>

namespace Hazel
{
    class GPUSamplerAsset;
    class GPUShaderAsset;
    class GPUTextureAsset;
    class MaterialAsset;
    class Renderer;

    class CachedMaterial : public GPUAsset
    {
    public:
        CachedMaterial() = delete;

        CachedMaterial(const UUID uuid,
                       uint64_t sourceVersion,
                       uint64_t materialID,
                       Renderer* renderer,
                       UUID shader,
                       uint64_t shaderSourceVersion,
                       const MaterialPipelineState& pipelineState,
                       const std::unordered_map<std::string, MaterialAssetProperty>& properties,
                       uint64_t lastReferencedFrame = 0)
            : GPUAsset(uuid, AssetType::Material, renderer, sourceVersion, lastReferencedFrame),
              m_IsValid(true),
              m_MaterialID(materialID),
              m_Shader(shader),
              m_ShaderSourceVersion(shaderSourceVersion),
              m_PipelineState(pipelineState),
              m_Properties(properties) {}

        uint32_t GetMaterialID() const
        {
            return m_MaterialID;
        }

        const std::unordered_map<std::string, MaterialAssetProperty>& GetProperties() const
        {
            return m_Properties;
        }

        const MaterialPipelineState& GetPipelineState() const
        {
            return m_PipelineState;
        }

        UUID GetShader() const
        {
            return m_Shader;
        }

        uint64_t GetShaderSourceVersion() const
        {
            return m_ShaderSourceVersion;
        }

        uint64_t GetPipelineKey(const std::vector<RHIFormat>& colorAttachmentFormats,
                                RHIFormat depthStencilFormat = RHIFormat::Undefined) const;

        bool IsDirty() const { return m_IsDirty; }
        void SetDirty(bool dirty) { m_IsDirty = dirty; }

        void Release() override;
        void ReleaseImmediate() override;;

    private:
        bool m_IsValid = false;
        bool m_IsDirty = true;
        uint32_t m_MaterialID = 0;
        Renderer* m_Renderer = nullptr;
        UUID m_Shader = UUID(-1);
        uint64_t m_ShaderSourceVersion = 0;
        MaterialPipelineState m_PipelineState{};
        std::unordered_map<std::string, MaterialAssetProperty> m_Properties;
    };
} // Hazel
