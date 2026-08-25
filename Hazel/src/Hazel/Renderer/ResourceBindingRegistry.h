// Declares shader resource and bindless binding management.
// Created: 2026-04-07.

#pragma once
#include "GPUAsset/GPUAssetHandle.h"
#include "GPUStructure.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"

namespace Hazel
{
    class Renderer;
}

namespace Aster
{
    constexpr int kBindlessRegistrySize = 65536;
    constexpr int kTextureBindingSlot = 1;
    constexpr int kSamplerBindingSlot = 2;
    constexpr int kCombinedImageSamplerBindingSlot = 3;

    struct PerViewUniformBufferInner
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewProj;
    };

    using PerViewUniformBuffer = Padded<PerViewUniformBufferInner, 256>;

    struct ShaderRegistryKey
    {
        Hazel::UUID uuid = Hazel::UUID(-1);
        uint64_t sourceVersion = 0;

        bool operator==(const ShaderRegistryKey& other) const
        { return uuid == other.uuid && sourceVersion == other.sourceVersion; }
    };

    struct ShaderRegistryKeyHash
    {
        size_t operator()(const ShaderRegistryKey& key) const
        {
            size_t seed = std::hash<Hazel::UUID>{}(key.uuid);
            seed ^= std::hash<uint64_t>{}(key.sourceVersion) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
            return seed;
        }
    };

    struct UserUploadValueBuffer
    {
        std::string name{};
        uint64_t version = 1;
        uint32_t size = 0;
        uint8_t data[64]{};

        template <typename T> void SetValue(const T& value)
        {
            std::memcpy(data, &value, sizeof(T));
            size = sizeof(T);
            ++version;
        }
    };

    struct UserUploadAssetBuffer
    {
        std::string name{};
        RHIBuffer* buffer = nullptr;
        RHIImageView* image = nullptr;
        RHISampler* sampler = nullptr;
    };

    struct UserUploadAssetMeta
    {
        enum class Type
        {
            Buffer,
            Sampler,
            SampledImage,
            StorageImage
        };
        std::string name{};
        Type type = Type::Buffer;
        uint32_t slot = 0;
        RHIBuffer* buffer = nullptr;
        RHIImageView* image = nullptr;
        RHISampler* sampler = nullptr;
    };

    struct UserUploadValueMeta
    {
        std::string name{};
        uint64_t version = 0;
        uint32_t offset = 0;
        uint32_t size = 0;
    };

    class ShaderMaterialSlot
    {
      public:
        ShaderMaterialSlot() = delete;

        ShaderMaterialSlot(Hazel::UUID shader,
                           uint64_t sourceVersion,
                           Hazel::Renderer* renderer,
                           const RHIShaderReflection& reflection);

        ~ShaderMaterialSlot();

        bool IsResized() const;
        void SetResized(bool resized);

        uint32_t RegisterMaterial(Hazel::UUID material);
        void UnregisterMaterial(uint32_t slot);
        std::vector<Hazel::UUID> GetMaterials() const;

        template <typename T>
        void
        SetUploadValueForFrame(const std::string& name, const T& value, uint64_t version, uint64_t frameInFlightIndex)
        { SetUploadValueForFrame(name, &value, sizeof(T), version, frameInFlightIndex); }

        void SetUploadValueForFrame(const std::string& name,
                                    const void* value,
                                    uint32_t valueSize,
                                    uint64_t version,
                                    uint64_t frameInFlightIndex);

        void SetUploadAssetForFrame(const std::string& name,
                                    const UserUploadAssetBuffer& asset,
                                    uint64_t frameInFlightIndex);

        void BuildSignature(const RHIShaderReflection& reflection);
        std::vector<GPUAssetHandle> BuildResources();
        std::vector<GPUAssetHandle> BuildResourcesForFrame(uint32_t frameInFlightIndex);

        RHIBuffer* GetMaterialBuffer() const;
        RHIBuffer* GetMaterialBuffer(uint32_t frameInFlightIndex) const;
        RHIResourceGroup* GetUserUploadResourceGroup() const;
        RHIResourceGroup* GetUserUploadResourceGroup(uint32_t frameInFlightIndex) const;
        RHIResourceLayout* GetUserUploadResourceLayout() const;
        RHIResourceGroup* GetMaterialResourceGroup() const;
        RHIResourceGroup* GetMaterialResourceGroup(uint32_t frameInFlightIndex) const;
        RHIResourceLayout* GetMaterialResourceLayout() const;
        const std::vector<RHIResourceLayout*>& GetShaderResourceLayouts() const;
        RHIResourceSignature* GetShaderResourceSignature() const;
        const RHIShaderReflection& GetShaderReflection() const;
        uint32_t GetUserUploadValueBufferSize() const;

      private:
        bool m_Resized = true;
        Hazel::UUID m_Shader = Hazel::UUID(-1);
        uint64_t m_SourceVersion = 0;
        Hazel::Renderer* m_Renderer = nullptr;
        uint32_t m_MaterialCount = 0;
        RHIShaderReflection m_Reflection{};
        std::vector<Hazel::UUID> m_Materials;
        std::vector<uint32_t> m_FreeList;
        std::vector<RHIBuffer*> m_MaterialBuffers;
        RHIBuffer* m_UserUploadValueBuffer = nullptr;
        uint32_t m_UserUploadValueBufferSize = 0;
        std::vector<std::unordered_map<std::string, UserUploadValueMeta>> m_UserUploadValueBufferMemberMap;
        std::vector<std::unordered_map<std::string, UserUploadAssetMeta>> m_UserUploadAssetBufferMap;
        RHIResourceLayout* m_UserUploadResourceLayout = nullptr;
        std::vector<RHIResourceGroup*> m_UserUploadResourceGroup;

        std::vector<RHIResourceGroup*> m_ResourceGroup;
        uint32_t m_MaterialStructSize = 0;
        std::vector<RHIResourceLayout*> m_ShaderResourceLayouts;
        RHIResourceSignature* m_ShaderResourceSignature = nullptr;
        RHIResourceLayout* m_MaterialResourceLayout = nullptr;

        std::vector<uint8_t> m_ShouldRebuild;
        std::vector<std::vector<uint8_t>> m_MaterialIsDirty;
    };

    class ResourceBindingRegistry
    {
      public:
        ResourceBindingRegistry(Hazel::Renderer* renderer);

        // per-view resources
        void CreatePerViewResources();
        void DestroyPerViewResources();

        RHIResourceGroup* GetPerViewResourceGroup() const;

        void SetViewProjectionMatrix(const glm::mat4& view, const glm::mat4& projection);

        void BindPerViewResources(RHICommandBuffer* cmd, Hazel::UUID shader, Hazel::UUID shaderVersion);
        void UpdateUserUploadDataForShader(Hazel::UUID shader, uint64_t sourceVersion);

        // bindless
        uint32_t RegisterTexture(GPUAssetHandle textureResult);
        uint32_t RegisterSampler(GPUAssetHandle samplerResult);
        uint32_t RegisterSamplerWithImage(GPUAssetHandle textureResult, GPUAssetHandle samplerResult);

        void UnregisterTexture(uint32_t index);
        void UnregisterSampler(uint32_t index);
        void UnregisterCombinedImageSampler(uint32_t index);

        const std::vector<GPUAssetHandle>& GetTextures() const { return m_Textures; }

        const std::vector<GPUAssetHandle>& GetSamplers() const { return m_Samplers; }

        const std::vector<std::pair<GPUAssetHandle, GPUAssetHandle>>& GetCombinedImageSamplers() const
        { return m_CombinedImageSamplers; }

        template <typename T> void SetValue(std::string name, const T& value)
        {
            if (!m_UserUploadValueBuffers.contains(name))
            {
                UserUploadValueBuffer buffer{};
                buffer.name = name;
                std::memcpy(buffer.data, &value, sizeof(T));
                m_UserUploadValueBuffers[name] = buffer;
            }

            auto& buffer = m_UserUploadValueBuffers[name];
            buffer.SetValue(value);
        }

        void SetBuffer(std::string name, const GPUAssetHandle* handle);
        void SetImage(std::string name, const GPUAssetHandle* handle);
        void SetSampler(std::string name, const GPUAssetHandle* handle);

        // per-shader resources
        void CreateOrUpdatePerShaderResources();
        void CreateOrUpdatePerShaderResourcesForShader(Hazel::UUID shader, uint64_t version);

        void RegisterShader(Hazel::UUID shader, uint64_t sourceVersion, const RHIShaderReflection& reflection);
        void UnregisterShader(Hazel::UUID shader, uint64_t sourceVersion);
        uint32_t RegisterMaterial(Hazel::UUID shader, uint64_t sourceVersion, Hazel::UUID material);
        void UnregisterMaterial(Hazel::UUID shader, uint64_t sourceVersion, uint32_t slot);

        std::vector<Hazel::UUID> GetMaterialsForShader(Hazel::UUID shader, uint64_t sourceVersion);
        RHIBuffer* GetMaterialBuffer(Hazel::UUID shader, uint64_t sourceVersion);
        RHIBuffer* GetMaterialBuffer(Hazel::UUID shader, uint64_t sourceVersion, uint32_t frameInFlightIndex);
        RHIResourceGroup* GetUserUploadResourceGroup(Hazel::UUID shader, uint64_t sourceVersion);
        RHIResourceGroup*
        GetUserUploadResourceGroup(Hazel::UUID shader, uint64_t sourceVersion, uint32_t frameInFlightIndex);
        RHIResourceLayout* GetUserUploadResourceLayout(Hazel::UUID shader, uint64_t sourceVersion);
        RHIResourceGroup* GetMaterialPropertyResourceGroup(Hazel::UUID shader, uint64_t sourceVersion);
        RHIResourceGroup*
        GetMaterialPropertyResourceGroup(Hazel::UUID shader, uint64_t sourceVersion, uint32_t frameInFlightIndex);
        RHIResourceLayout* GetMaterialPropertyResourceLayout(Hazel::UUID shader, uint64_t sourceVersion);
        const std::vector<RHIResourceLayout*>* GetShaderResourceLayouts(Hazel::UUID shader, uint64_t sourceVersion);
        RHIResourceSignature* GetShaderResourceSignature(Hazel::UUID shader, uint64_t sourceVersion);
        void BindMaterialPropertyResources(RHICommandBuffer* cmd, Hazel::UUID shader, uint64_t sourceVersion);
        void BindUserUploadResources(RHICommandBuffer* cmd, Hazel::UUID shader, uint64_t sourceVersion);

        void ClearAllResources();

      private:
        Hazel::Renderer* m_Renderer = nullptr;

        // per-view
        RHIResourceLayout* m_PerViewResourceLayout = nullptr;
        RHIResourceGroup* m_PerViewResourceGroup = nullptr;
        RHIBuffer* m_PerViewUniformBuffer = nullptr;

        // shader-material mapping
        std::mutex m_ShaderMaterialMutex;
        std::unordered_map<ShaderRegistryKey, std::unique_ptr<ShaderMaterialSlot>, ShaderRegistryKeyHash>
            m_ShaderMaterials;

        // bindless mapping
        std::mutex m_TextureMutex;
        std::vector<GPUAssetHandle> m_Textures;
        std::vector<uint32_t> m_TextureFreeList;
        std::vector<uint8_t> m_TextureFreeMap;

        std::mutex m_SamplerMutex;
        std::vector<GPUAssetHandle> m_Samplers;
        std::vector<uint32_t> m_SamplerFreeList;
        std::vector<uint8_t> m_SamplerFreeMap;

        std::mutex m_CombinedImageSamplerMutex;
        std::vector<std::pair<GPUAssetHandle, GPUAssetHandle>> m_CombinedImageSamplers;
        std::vector<uint32_t> m_CombinedImageSamplerFreeList;
        std::vector<uint8_t> m_CombinedImageSamplerFreeMap;

        std::unordered_map<std::string, UserUploadValueBuffer> m_UserUploadValueBuffers;
        std::unordered_map<std::string, UserUploadAssetBuffer> m_UserUploadAssetBuffers;
    };
} // namespace Aster
