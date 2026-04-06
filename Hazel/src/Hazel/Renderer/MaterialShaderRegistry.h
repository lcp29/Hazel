//
// Created by helmholtz on 2026/4/5.
//

#pragma once
#include "GPUAsset/CachedMaterial.h"
#include "GPUAsset/GPUAssetResolveResult.h"
#include "Hazel/RHI/RHI.h"

#include <mutex>
#include <unordered_map>
#include <vector>

namespace Hazel
{
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

        ShaderMaterialSlot(UUID shader, uint64_t sourceVersion, Renderer* renderer, const RHIShaderReflection& reflection);

        ~ShaderMaterialSlot();

        bool IsResized() const { return m_Resized; }
        void SetResized(bool resized) { m_Resized = resized; }

        uint32_t RegisterMaterial(UUID material)
        {
            if (material == UUID(-1))
            {
                return -1;
            }
            if (!m_FreeList.empty())
            {
                uint32_t slot = m_FreeList.back();
                m_FreeList.pop_back();
                m_Materials[slot] = material;
                for (auto& dirtySlot : m_MaterialIsDirty)
                {
                    dirtySlot[slot] = true;
                }
                m_MaterialCount++;
                return slot;
            }
            else
            {
                m_Materials.push_back(material);
                m_Resized = true;
                for (auto& dirtySlot : m_MaterialIsDirty)
                {
                    dirtySlot.push_back(true);
                }
                return m_MaterialCount++;
            }
        }

        void UnregisterMaterial(uint32_t slot)
        {
            if (slot == -1)
            {
                return;
            }
            if (slot < m_Materials.size())
            {
                if (m_Materials[slot] != UUID(-1))
                {
                    m_Materials[slot] = UUID(-1);
                    m_FreeList.push_back(slot);
                    m_MaterialCount--;
                }
            }
        }

        std::vector<UUID> GetMaterials() const
        {
            std::vector<UUID> materials;
            for (const auto& material : m_Materials)
            {
                if (material != UUID(-1))
                {
                    materials.push_back(material);
                }
            }
            return materials;
        }

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
        const RHIShaderReflection& GetShaderReflection() const { return m_Reflection; }

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

    class MaterialShaderRegistry
    {
    public:
        MaterialShaderRegistry() = delete;

        MaterialShaderRegistry(Renderer* renderer)
            : m_Renderer(renderer) {}

        void RegisterShader(UUID shader, uint64_t sourceVersion, const RHIShaderReflection& reflection)
        {
            std::unique_lock lock(m_ShaderMaterialMutex);
            ShaderRegistryKey key{shader, sourceVersion};
            if (!m_ShaderMaterials.contains(key))
            {
                m_ShaderMaterials.emplace(
                    key,
                    std::make_unique<ShaderMaterialSlot>(shader, sourceVersion, m_Renderer, reflection));
            }
        }

        void UnregisterShader(UUID shader, uint64_t sourceVersion)
        {
            std::unique_lock shaderMaterialLock(m_ShaderMaterialMutex);
            m_ShaderMaterials.erase({shader, sourceVersion});
        }

        uint32_t RegisterMaterial(UUID shader, uint64_t sourceVersion, UUID material)
        {
            std::unique_lock lock(m_ShaderMaterialMutex);
            ShaderRegistryKey key{shader, sourceVersion};
            if (m_ShaderMaterials.contains(key))
            {
                return m_ShaderMaterials[key]->RegisterMaterial(material);
            }
            return -1;
        }

        void UnregisterMaterial(UUID shader, uint64_t sourceVersion, uint32_t slot)
        {
            std::unique_lock lock(m_ShaderMaterialMutex);
            ShaderRegistryKey key{shader, sourceVersion};
            if (m_ShaderMaterials.contains(key))
            {
                m_ShaderMaterials[key]->UnregisterMaterial(slot);
            }
        }

        std::vector<UUID> GetMaterialsForShader(UUID shader, uint64_t sourceVersion)
        {
            std::unique_lock lock(m_ShaderMaterialMutex);
            ShaderRegistryKey key{shader, sourceVersion};
            if (m_ShaderMaterials.contains(key))
            {
                return m_ShaderMaterials.at(key)->GetMaterials();
            }
            return {};
        }

        RHIBuffer* GetMaterialBuffer(UUID shader, uint64_t sourceVersion)
        {
            std::unique_lock lock(m_ShaderMaterialMutex);
            auto it = m_ShaderMaterials.find({shader, sourceVersion});
            if (it != m_ShaderMaterials.end())
            {
                return it->second->GetMaterialBuffer();
            }
            return {};
        }

        RHIBuffer* GetMaterialBuffer(UUID shader, uint64_t sourceVersion, uint32_t frameInFlightIndex)
        {
            std::unique_lock lock(m_ShaderMaterialMutex);
            auto it = m_ShaderMaterials.find({shader, sourceVersion});
            if (it != m_ShaderMaterials.end())
            {
                return it->second->GetMaterialBuffer(frameInFlightIndex);
            }
            return {};
        }

        RHIResourceGroup* GetMaterialResourceGroup(UUID shader, uint64_t sourceVersion)
        {
            std::unique_lock lock(m_ShaderMaterialMutex);
            auto it = m_ShaderMaterials.find({shader, sourceVersion});
            if (it != m_ShaderMaterials.end())
            {
                return it->second->GetMaterialResourceGroup();
            }
            return {};
        }

        RHIResourceGroup* GetMaterialResourceGroup(UUID shader, uint64_t sourceVersion, uint32_t frameInFlightIndex)
        {
            std::unique_lock lock(m_ShaderMaterialMutex);
            auto it = m_ShaderMaterials.find({shader, sourceVersion});
            if (it != m_ShaderMaterials.end())
            {
                return it->second->GetMaterialResourceGroup(frameInFlightIndex);
            }
            return {};
        }

        RHIResourceLayout* GetMaterialResourceLayout(UUID shader, uint64_t sourceVersion)
        {
            std::unique_lock lock(m_ShaderMaterialMutex);
            auto it = m_ShaderMaterials.find({shader, sourceVersion});
            if (it != m_ShaderMaterials.end())
            {
                return it->second->GetMaterialResourceLayout();
            }
            return {};
        }

        const std::vector<RHIResourceLayout*>* GetShaderResourceLayouts(UUID shader, uint64_t sourceVersion)
        {
            std::unique_lock lock(m_ShaderMaterialMutex);
            auto it = m_ShaderMaterials.find({shader, sourceVersion});
            if (it != m_ShaderMaterials.end())
            {
                return &it->second->GetShaderResourceLayouts();
            }
            return nullptr;
        }

        RHIResourceSignature* GetShaderResourceSignature(UUID shader, uint64_t sourceVersion)
        {
            std::unique_lock lock(m_ShaderMaterialMutex);
            auto it = m_ShaderMaterials.find({shader, sourceVersion});
            if (it != m_ShaderMaterials.end())
            {
                return it->second->GetShaderResourceSignature();
            }
            return {};
        }

    private:
        Renderer* m_Renderer;
        std::mutex m_ShaderMaterialMutex;
        std::unordered_map<ShaderRegistryKey,
                           std::unique_ptr<ShaderMaterialSlot>,
                           ShaderRegistryKeyHash> m_ShaderMaterials;
    };
}
