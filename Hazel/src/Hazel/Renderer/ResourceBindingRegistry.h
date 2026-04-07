//
// Created by helmholtz on 2026/4/7.
//

#pragma once
#include "Hazel/RHI/RHI.h"
#include "GPUAsset/GPUAssetResolveResult.h"
#include "Hazel/Core/UUID.h"

namespace Hazel
{
    constexpr int kBindlessRegistrySize = 65536;
    constexpr int kTextureBindingSlot = 1;
    constexpr int kSamplerBindingSlot = 2;
    constexpr int kCombinedImageSamplerBindingSlot = 3;

    class Renderer;

    struct ShaderRegistryKey
    {
        UUID uuid = UUID(-1);
        uint64_t sourceVersion = 0;

        bool operator==(const ShaderRegistryKey& other) const
        {
            return uuid == other.uuid && sourceVersion == other.sourceVersion;
        }
    };

    struct ShaderRegistryKeyHash
    {
        size_t operator()(const ShaderRegistryKey& key) const
        {
            size_t seed = std::hash<UUID>{}(key.uuid);
            seed ^= std::hash<uint64_t>{}(key.sourceVersion) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
            return seed;
        }
    };

    class ShaderMaterialSlot
    {
    public:
        ShaderMaterialSlot() = delete;

        ShaderMaterialSlot(UUID shader,
                           uint64_t sourceVersion,
                           Renderer* renderer,
                           const RHIShaderReflection& reflection);

        ~ShaderMaterialSlot();

        bool IsResized() const;
        void SetResized(bool resized);

        uint32_t RegisterMaterial(UUID material);

        void UnregisterMaterial(uint32_t slot);

        std::vector<UUID> GetMaterials() const;

        void BuildSignature(const RHIShaderReflection& reflection);
        std::vector<GPUAssetResolveResult> BuildResources();
        std::vector<GPUAssetResolveResult> BuildResourcesForFrame(uint32_t frameInFlightIndex);
        RHIBuffer* GetMaterialBuffer() const;
        RHIBuffer* GetMaterialBuffer(uint32_t frameInFlightIndex) const;
        RHIResourceGroup* GetMaterialResourceGroup() const;
        RHIResourceGroup* GetMaterialResourceGroup(uint32_t frameInFlightIndex) const;
        RHIResourceLayout* GetMaterialResourceLayout() const;
        const std::vector<RHIResourceLayout*>& GetShaderResourceLayouts() const;
        RHIResourceSignature* GetShaderResourceSignature() const;
        const RHIShaderReflection& GetShaderReflection() const;

    private:
        bool m_Resized = true;
        UUID m_Shader = UUID(-1);
        uint64_t m_SourceVersion = 0;
        Renderer* m_Renderer = nullptr;
        uint32_t m_MaterialCount = 0;
        RHIShaderReflection m_Reflection{};
        std::vector<UUID> m_Materials;
        std::vector<uint32_t> m_FreeList;
        std::vector<RHIBuffer*> m_MaterialBuffer;
        std::vector<RHIResourceGroup*> m_ResourceGroup;

        uint32_t m_MaterialStructSize = 0;
        std::vector<RHIResourceLayout*> m_ShaderResourceLayouts;
        RHIResourceSignature* m_ShaderResourceSignature = nullptr;
        RHIResourceLayout* m_MaterialResourceLayout = nullptr;

        std::vector<uint8_t> m_ShouldRebuild;
        std::vector<std::vector<uint8_t>> m_MaterialIsDirty;
    };

    struct PendingResourceOperation
    {
        enum class SlotType
        {
            Texture,
            Sampler,
            CombinedImageSampler
        };

        SlotType type;
        uint32_t slot;
        GPUAsset* texture = nullptr;
        GPUAsset* sampler = nullptr;
    };

    class ResourceBindingRegistry
    {
    public:
        ResourceBindingRegistry(Renderer* renderer);

        // per-view resources
        void CreatePerViewResources();
        void DestroyPerViewResources();

        RHIResourceGroup* GetPerViewResourceGroup() const;

        void BindPerViewResources(RHICommandBuffer* cmd, RHIGraphicsPipeline* pipeline);

        // bindless
        uint32_t RegisterTexture(GPUAssetResolveResult textureResult);
        uint32_t RegisterSampler(GPUAssetResolveResult samplerResult);
        uint32_t RegisterSamplerWithImage(GPUAssetResolveResult textureResult,
                                          GPUAssetResolveResult samplerResult);

        void UnregisterTexture(uint32_t index);
        void UnregisterSampler(uint32_t index);
        void UnregisterCombinedImageSampler(uint32_t index);

        const std::vector<GPUAssetResolveResult>& GetTextures() const { return m_Textures; }
        const std::vector<GPUAssetResolveResult>& GetSamplers() const { return m_Samplers; }

        const std::vector<std::pair<GPUAssetResolveResult, GPUAssetResolveResult>>& GetCombinedImageSamplers() const
        {
            return m_CombinedImageSamplers;
        }

        void UpdateResourceGroupForPendingOperations(RHIResourceGroup* group);

        // material property resources
        void CreateOrUpdateMaterialPropertyResources();

        void RegisterShader(UUID shader, uint64_t sourceVersion, const RHIShaderReflection& reflection);
        void UnregisterShader(UUID shader, uint64_t sourceVersion);
        uint32_t RegisterMaterial(UUID shader, uint64_t sourceVersion, UUID material);
        void UnregisterMaterial(UUID shader, uint64_t sourceVersion, uint32_t slot);

        std::vector<UUID> GetMaterialsForShader(UUID shader, uint64_t sourceVersion);
        RHIBuffer* GetMaterialPropertyBuffer(UUID shader, uint64_t sourceVersion);
        RHIBuffer* GetMaterialPropertyBuffer(UUID shader, uint64_t sourceVersion, uint32_t frameInFlightIndex);
        RHIResourceGroup* GetMaterialPropertyResourceGroup(UUID shader, uint64_t sourceVersion);
        RHIResourceGroup* GetMaterialPropertyResourceGroup(UUID shader,
                                                           uint64_t sourceVersion,
                                                           uint32_t frameInFlightIndex);
        RHIResourceLayout* GetMaterialPropertyResourceLayout(UUID shader, uint64_t sourceVersion);
        const std::vector<RHIResourceLayout*>* GetShaderResourceLayouts(UUID shader, uint64_t sourceVersion);
        RHIResourceSignature* GetShaderResourceSignature(UUID shader, uint64_t sourceVersion);
        void BindMaterialPropertyResources(RHICommandBuffer* cmd,
                                           RHIGraphicsPipeline* pipeline,
                                           UUID shader,
                                           uint64_t sourceVersion);



    private:
        Renderer* m_Renderer = nullptr;

        // per-view
        RHIResourceLayout* m_PerViewResourceLayout = nullptr;
        RHIResourceGroup* m_PerViewResourceGroup = nullptr;
        RHIBuffer* m_PerViewUniformBuffer = nullptr;

        // shader-material mapping
        std::mutex m_ShaderMaterialMutex;
        std::unordered_map<ShaderRegistryKey,
                           std::unique_ptr<ShaderMaterialSlot>,
                           ShaderRegistryKeyHash> m_ShaderMaterials;

        // bindless mapping
        std::mutex m_TextureMutex;
        std::vector<GPUAssetResolveResult> m_Textures;
        std::vector<uint32_t> m_TextureFreeList;
        std::vector<uint8_t> m_TextureFreeMap;

        std::mutex m_SamplerMutex;
        std::vector<GPUAssetResolveResult> m_Samplers;
        std::vector<uint32_t> m_SamplerFreeList;
        std::vector<uint8_t> m_SamplerFreeMap;

        std::mutex m_CombinedImageSamplerMutex;
        std::vector<std::pair<GPUAssetResolveResult, GPUAssetResolveResult>> m_CombinedImageSamplers;
        std::vector<uint32_t> m_CombinedImageSamplerFreeList;
        std::vector<uint8_t> m_CombinedImageSamplerFreeMap;

        std::mutex m_PendingResourceOperationMutex;
        std::vector<PendingResourceOperation> m_PendingResourceOperations;
    };
} // Hazel