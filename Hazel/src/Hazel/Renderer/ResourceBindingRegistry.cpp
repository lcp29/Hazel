//
// Created by helmholtz on 2026/4/7.
//

#include "ResourceBindingRegistry.h"
#include "Renderer.h"
#include "GPUStructure.h"
#include "ShaderCommon.h"
#include "GPUAsset/CachedMaterial.h"

namespace Hazel
{
    ShaderMaterialSlot::ShaderMaterialSlot(UUID shader,
                                           uint64_t sourceVersion,
                                           Renderer* renderer,
                                           const RHIShaderReflection& reflection)
        : m_Shader(shader), m_SourceVersion(sourceVersion), m_Renderer(renderer), m_Reflection(reflection)
    {
        uint32_t maxFramesInFlight = renderer->GetMaxFramesInFlight();
        m_MaterialBuffer.resize(maxFramesInFlight, nullptr);
        m_ResourceGroup.resize(maxFramesInFlight, nullptr);
        m_ShaderResourceLayouts.resize(maxFramesInFlight);
        m_ShouldRebuild.resize(maxFramesInFlight, false);
        m_MaterialIsDirty.resize(maxFramesInFlight);
        BuildSignature(reflection);
    }

    ShaderMaterialSlot::~ShaderMaterialSlot()
    {
        for (auto* resourceGroup : m_ResourceGroup)
        {
            if (resourceGroup)
            {
                m_Renderer->GetResourceHeapAllocator()->FreeGroup(resourceGroup);
            }
        }

        for (auto layout : m_ShaderResourceLayouts)
        {
            if (layout)
            {
                layout->ReleaseImmediate();
            }
        }

        if (m_ShaderResourceSignature)
        {
            m_ShaderResourceSignature->ReleaseImmediate();
        }

        for (auto* materialBuffer : m_MaterialBuffer)
        {
            if (materialBuffer)
            {
                materialBuffer->ReleaseImmediate();
            }
        }
    }

    bool ShaderMaterialSlot::IsResized() const { return m_Resized; }

    void ShaderMaterialSlot::SetResized(bool resized) { m_Resized = resized; }

    uint32_t ShaderMaterialSlot::RegisterMaterial(UUID material)
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

    void ShaderMaterialSlot::UnregisterMaterial(uint32_t slot)
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

    std::vector<UUID> ShaderMaterialSlot::GetMaterials() const
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

    const RHIShaderReflection& ShaderMaterialSlot::GetShaderReflection() const { return m_Reflection; }

    void ShaderMaterialSlot::BuildSignature(const RHIShaderReflection& reflection)
    {
        if (m_ShaderResourceSignature)
        {
            m_ShaderResourceSignature->ReleaseImmediate();
            m_ShaderResourceSignature = nullptr;
        }

        for (auto* resourceLayout : m_ShaderResourceLayouts)
        {
            if (resourceLayout)
            {
                resourceLayout->ReleaseImmediate();
            }
        }
        m_ShaderResourceLayouts.clear();
        m_MaterialResourceLayout = nullptr;

        std::vector<RHIResourceLayoutDesc> setData;
        AddReflectionToSetData(
            setData,
            reflection,
            RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment);

        RHIResourceLayoutDesc materialPropertiesLayoutDesc{};
        materialPropertiesLayoutDesc.bindings.emplace_back(RHIResourceBindingSlotDesc{
            .slot = 0,
            .type = RHIResourceBindingType::StorageBuffer,
            .count = 1,
            .stages = RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment,});

        setData[kMaterialResourceSet] = materialPropertiesLayoutDesc;

        m_ShaderResourceLayouts.reserve(setData.size());
        for (uint32_t set = 0; set < setData.size(); set++)
        {
            auto& layoutDesc = setData[set];

            if (layoutDesc.bindings.empty())
            {
                m_ShaderResourceLayouts.push_back(nullptr);
                continue;
            }

            std::ranges::sort(layoutDesc.bindings,
                              [](const RHIResourceBindingSlotDesc& lhs, const RHIResourceBindingSlotDesc& rhs) {
                                  return lhs.slot < rhs.slot;
                              });

            auto* layout = m_Renderer->GetDevice()->CreateResourceLayout(layoutDesc);
            m_ShaderResourceLayouts.push_back(layout);
            if (set == kMaterialResourceSet)
            {
                m_MaterialResourceLayout = layout;
            }
        }

        std::vector<RHIPushConstantRangeDesc> pushConstantRanges;
        pushConstantRanges.reserve(reflection.pushConstants.size());
        for (const auto& pushConstant : reflection.pushConstants)
        {
            RHIPushConstantRangeDesc range{};
            range.offset = pushConstant.offset;
            range.size = pushConstant.size;
            range.stages = RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment;
            pushConstantRanges.push_back(range);
        }

        RHIResourceSignatureDesc signatureDesc{};
        signatureDesc.resourceLayouts = m_ShaderResourceLayouts;
        signatureDesc.pushConstantRanges = std::move(pushConstantRanges);
        m_ShaderResourceSignature =
            m_Renderer->GetDevice()->CreateResourceSignature(signatureDesc);
    }

    std::vector<GPUAssetResolveResult> ShaderMaterialSlot::BuildResources()
    {
        return BuildResourcesForFrame(m_Renderer->GetCurrentFrameInFlightIndex());
    }

    std::vector<GPUAssetResolveResult> ShaderMaterialSlot::BuildResourcesForFrame(
        uint32_t frameInFlightIndex)
    {
        std::vector<GPUAssetResolveResult> resourceResults;

        if (m_Resized)
        {
            std::ranges::fill(m_ShouldRebuild, true);
            m_Resized = false;
        }

        const auto& reflection = m_Reflection;

        if (m_ShouldRebuild[frameInFlightIndex])
        {
            if (m_ResourceGroup[frameInFlightIndex])
            {
                m_Renderer->GetResourceHeapAllocator()->FreeGroup(m_ResourceGroup[frameInFlightIndex]);
                m_ResourceGroup[frameInFlightIndex] = nullptr;
            }

            if (m_MaterialBuffer[frameInFlightIndex])
            {
                m_MaterialBuffer[frameInFlightIndex]->ReleaseImmediate();
                m_MaterialBuffer[frameInFlightIndex] = nullptr;
            }
            m_MaterialStructSize = 0;

            std::vector<RHIResourceLayoutDesc> setData;
            AddReflectionToSetData(
                setData,
                reflection,
                RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment);

            for (const auto& group : reflection.resourceGroups)
            {
                if (group.set != kMaterialResourceSet)
                {
                    continue;
                }

                for (const auto& slot : group.slots)
                {
                    if (slot.slot == 0)
                    {
                        m_MaterialStructSize = slot.buffer.size;
                        break;
                    }
                }
                break;
            }

            RHIBufferDesc bufferDesc{};
            bufferDesc.size = static_cast<uint64_t>(m_MaterialStructSize) * m_Materials.size();
            bufferDesc.usages = RHIBufferUsageFlagBits::StorageBuffer;
            bufferDesc.allowGpuAddress = false;
            bufferDesc.cpuAccess = RHIBufferCpuAccess::Write;
            bufferDesc.hostCoherent = true;
            bufferDesc.mapOnCreate = true;
            m_MaterialBuffer[frameInFlightIndex] = m_Renderer->GetDevice()->CreateBuffer(bufferDesc);

            RHIResourceHeap* materialHeap = nullptr;
            m_ResourceGroup[frameInFlightIndex] = m_Renderer->GetResourceHeapAllocator()->AllocateGroup(
                m_MaterialResourceLayout,
                &materialHeap);
            m_ResourceGroup[frameInFlightIndex]->WriteBuffer(0,
                                                             m_MaterialBuffer[frameInFlightIndex],
                                                             0,
                                                             m_MaterialBuffer[frameInFlightIndex]->GetSize());

            auto* mappedBuffer = static_cast<uint8_t*>(m_MaterialBuffer[frameInFlightIndex]->Map());
            std::memset(mappedBuffer, 0, m_MaterialBuffer[frameInFlightIndex]->GetSize());
        }

        auto* mappedBuffer = static_cast<uint8_t*>(m_MaterialBuffer[frameInFlightIndex]->Map());
        for (const auto& materialUUID : m_Materials)
        {
            if (materialUUID == UUID(-1))
            {
                continue;
            }

            auto materialResult = m_Renderer->ResolveGPUAsset(materialUUID, AssetType::Material);
            auto* material = static_cast<CachedMaterial*>(materialResult.asset);
            if (!material)
            {
                continue;
            }

            const auto materialID = material->GetMaterialID();

            if (material->IsDirty())
            {
                for (auto& dirtySlot : m_MaterialIsDirty)
                {
                    dirtySlot[materialID] = true;
                }
                material->SetDirty(false);
            }

            const bool shouldWriteMaterial = m_ShouldRebuild[frameInFlightIndex] ||
                                             m_MaterialIsDirty[frameInFlightIndex][materialID];
            auto* materialData = mappedBuffer + static_cast<uint64_t>(materialID) * m_MaterialStructSize;
            if (shouldWriteMaterial)
            {
                std::memset(materialData, 0, m_MaterialStructSize);
                for (const auto& property : material->GetProperties() | std::views::values)
                {
                    std::memcpy(materialData + property.member.offset, property.data, property.member.size);
                }
            }

            if (shouldWriteMaterial)
            {
                m_MaterialIsDirty[frameInFlightIndex][materialID] = false;
            }
        }

        if (m_ShouldRebuild[frameInFlightIndex])
        {
            m_ShouldRebuild[frameInFlightIndex] = false;
        }

        return resourceResults;
    }

    RHIBuffer* ShaderMaterialSlot::GetMaterialBuffer() const
    {
        return GetMaterialBuffer(m_Renderer->GetCurrentFrameInFlightIndex());
    }

    RHIBuffer* ShaderMaterialSlot::GetMaterialBuffer(uint32_t frameInFlightIndex) const
    {
        return m_MaterialBuffer[frameInFlightIndex];
    }

    RHIResourceGroup* ShaderMaterialSlot::GetMaterialResourceGroup() const
    {
        return GetMaterialResourceGroup(m_Renderer->GetCurrentFrameInFlightIndex());
    }

    RHIResourceGroup* ShaderMaterialSlot::GetMaterialResourceGroup(uint32_t frameInFlightIndex) const
    {
        return m_ResourceGroup[frameInFlightIndex];
    }

    RHIResourceLayout* ShaderMaterialSlot::GetMaterialResourceLayout() const
    {
        return m_MaterialResourceLayout;
    }

    const std::vector<RHIResourceLayout*>& ShaderMaterialSlot::GetShaderResourceLayouts() const
    {
        return m_ShaderResourceLayouts;
    }

    RHIResourceSignature* ShaderMaterialSlot::GetShaderResourceSignature() const
    {
        return m_ShaderResourceSignature;
    }

    ResourceBindingRegistry::ResourceBindingRegistry(Renderer* renderer)
        : m_Renderer(renderer)
    {
        m_TextureFreeMap.resize(kBindlessRegistrySize, true);
        m_SamplerFreeMap.resize(kBindlessRegistrySize, true);
        m_CombinedImageSamplerFreeMap.resize(kBindlessRegistrySize, true);
    }

    RHIResourceGroup* ResourceBindingRegistry::GetPerViewResourceGroup() const
    {
        return m_PerViewResourceGroup;
    }

    void ResourceBindingRegistry::RegisterShader(UUID shader,
                                                 uint64_t sourceVersion,
                                                 const RHIShaderReflection& reflection)
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

    void ResourceBindingRegistry::UnregisterShader(UUID shader, uint64_t sourceVersion)
    {
        std::unique_lock shaderMaterialLock(m_ShaderMaterialMutex);
        m_ShaderMaterials.erase({shader, sourceVersion});
    }

    uint32_t ResourceBindingRegistry::RegisterMaterial(UUID shader, uint64_t sourceVersion, UUID material)
    {
        std::unique_lock lock(m_ShaderMaterialMutex);
        ShaderRegistryKey key{shader, sourceVersion};
        if (m_ShaderMaterials.contains(key))
        {
            return m_ShaderMaterials[key]->RegisterMaterial(material);
        }
        return -1;
    }

    void ResourceBindingRegistry::UnregisterMaterial(UUID shader, uint64_t sourceVersion, uint32_t slot)
    {
        std::unique_lock lock(m_ShaderMaterialMutex);
        ShaderRegistryKey key{shader, sourceVersion};
        if (m_ShaderMaterials.contains(key))
        {
            m_ShaderMaterials[key]->UnregisterMaterial(slot);
        }
    }

    std::vector<UUID> ResourceBindingRegistry::GetMaterialsForShader(UUID shader, uint64_t sourceVersion)
    {
        std::unique_lock lock(m_ShaderMaterialMutex);
        ShaderRegistryKey key{shader, sourceVersion};
        if (m_ShaderMaterials.contains(key))
        {
            return m_ShaderMaterials.at(key)->GetMaterials();
        }
        return {};
    }

    RHIBuffer* ResourceBindingRegistry::GetMaterialPropertyBuffer(UUID shader, uint64_t sourceVersion)
    {
        std::unique_lock lock(m_ShaderMaterialMutex);
        auto it = m_ShaderMaterials.find({shader, sourceVersion});
        if (it != m_ShaderMaterials.end())
        {
            return it->second->GetMaterialBuffer();
        }
        return {};
    }

    RHIBuffer* ResourceBindingRegistry::GetMaterialPropertyBuffer(UUID shader,
                                                                  uint64_t sourceVersion,
                                                                  uint32_t frameInFlightIndex)
    {
        std::unique_lock lock(m_ShaderMaterialMutex);
        auto it = m_ShaderMaterials.find({shader, sourceVersion});
        if (it != m_ShaderMaterials.end())
        {
            return it->second->GetMaterialBuffer(frameInFlightIndex);
        }
        return {};
    }

    RHIResourceGroup* ResourceBindingRegistry::GetMaterialPropertyResourceGroup(UUID shader, uint64_t sourceVersion)
    {
        std::unique_lock lock(m_ShaderMaterialMutex);
        auto it = m_ShaderMaterials.find({shader, sourceVersion});
        if (it != m_ShaderMaterials.end())
        {
            return it->second->GetMaterialResourceGroup();
        }
        return {};
    }

    RHIResourceGroup* ResourceBindingRegistry::GetMaterialPropertyResourceGroup(UUID shader,
        uint64_t sourceVersion,
        uint32_t frameInFlightIndex)
    {
        std::unique_lock lock(m_ShaderMaterialMutex);
        auto it = m_ShaderMaterials.find({shader, sourceVersion});
        if (it != m_ShaderMaterials.end())
        {
            return it->second->GetMaterialResourceGroup(frameInFlightIndex);
        }
        return {};
    }

    RHIResourceLayout* ResourceBindingRegistry::GetMaterialPropertyResourceLayout(UUID shader, uint64_t sourceVersion)
    {
        std::unique_lock lock(m_ShaderMaterialMutex);
        auto it = m_ShaderMaterials.find({shader, sourceVersion});
        if (it != m_ShaderMaterials.end())
        {
            return it->second->GetMaterialResourceLayout();
        }
        return {};
    }

    const std::vector<RHIResourceLayout*>* ResourceBindingRegistry::GetShaderResourceLayouts(UUID shader,
        uint64_t sourceVersion)
    {
        std::unique_lock lock(m_ShaderMaterialMutex);
        auto it = m_ShaderMaterials.find({shader, sourceVersion});
        if (it != m_ShaderMaterials.end())
        {
            return &it->second->GetShaderResourceLayouts();
        }
        return nullptr;
    }

    RHIResourceSignature* ResourceBindingRegistry::GetShaderResourceSignature(UUID shader, uint64_t sourceVersion)
    {
        std::unique_lock lock(m_ShaderMaterialMutex);
        auto it = m_ShaderMaterials.find({shader, sourceVersion});
        if (it != m_ShaderMaterials.end())
        {
            return it->second->GetShaderResourceSignature();
        }
        return {};
    }

    void ResourceBindingRegistry::BindMaterialPropertyResources(RHICommandBuffer* cmd,
                                                                RHIGraphicsPipeline* pipeline,
                                                                UUID shader,
                                                                uint64_t sourceVersion)
    {
        if (auto* resourceGroup = GetMaterialPropertyResourceGroup(shader, sourceVersion))
        {
            cmd->BindGraphicsResourceGroup(pipeline, kMaterialResourceSet, resourceGroup);
        }
    }

    void ResourceBindingRegistry::CreateOrUpdateMaterialPropertyResources()
    {
        for (auto& slot : m_ShaderMaterials | std::views::values)
        {
            slot->BuildResources();
        }
    }

    void ResourceBindingRegistry::CreatePerViewResources()
    {
        DestroyPerViewResources();

        RHIResourceLayoutDesc layoutDesc{};
        layoutDesc.bindings.emplace_back(0,
                                         RHIResourceBindingType::UniformDynamicBuffer,
                                         1,
                                         RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment);
        layoutDesc.bindings.emplace_back(1,
                                         RHIResourceBindingType::SampledImage,
                                         kBindlessRegistrySize,
                                         RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment);
        layoutDesc.bindings.emplace_back(2,
                                         RHIResourceBindingType::Sampler,
                                         kBindlessRegistrySize,
                                         RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment);
        layoutDesc.bindings.emplace_back(3,
                                         RHIResourceBindingType::SamplerWithImage,
                                         kBindlessRegistrySize,
                                         RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment);

        m_PerViewResourceLayout = m_Renderer->GetDevice()->CreateResourceLayout(layoutDesc);
        m_PerViewResourceGroup = m_Renderer->GetResourceHeapAllocator()->
                                             AllocateGroup(m_PerViewResourceLayout, nullptr);

        RHIBufferDesc bufferDesc{};
        bufferDesc.size = sizeof(PerViewUniformBuffer) * m_Renderer->GetMaxFramesInFlight();
        bufferDesc.usages = RHIBufferUsageFlagBits::UniformBuffer;
        bufferDesc.allowGpuAddress = false;
        bufferDesc.cpuAccess = RHIBufferCpuAccess::Write;
        bufferDesc.hostCoherent = true;
        bufferDesc.mapOnCreate = true;
        m_PerViewUniformBuffer = m_Renderer->GetDevice()->CreateBuffer(bufferDesc);
    }

    void ResourceBindingRegistry::DestroyPerViewResources()
    {
        if (m_PerViewResourceGroup)
        {
            m_Renderer->GetResourceHeapAllocator()->FreeGroup(m_PerViewResourceGroup);
        }

        if (m_PerViewResourceLayout)
        {
            m_PerViewResourceLayout->ReleaseImmediate();
        }

        if (m_PerViewUniformBuffer)
        {
            m_PerViewUniformBuffer->ReleaseImmediate();
        }
    }

    void ResourceBindingRegistry::BindPerViewResources(RHICommandBuffer* cmd, RHIGraphicsPipeline* pipeline)
    {
        std::vector offsets = {
            static_cast<uint32_t>(m_Renderer->GetCurrentFrameInFlightIndex() * sizeof(PerViewUniformBuffer))};
        cmd->BindGraphicsResourceGroup(pipeline,
                                       kPerViewResourceSet,
                                       GetPerViewResourceGroup(),
                                       &offsets);
    }

    uint32_t ResourceBindingRegistry::RegisterTexture(GPUAssetResolveResult textureResult)
    {
        if (!textureResult.asset)
        {
            return -1;
        }

        auto* asset = textureResult.asset;

        std::unique_lock textureLock(m_TextureMutex);
        std::unique_lock operationLock(m_PendingResourceOperationMutex);

        if (!m_TextureFreeList.empty())
        {
            const uint32_t index = m_TextureFreeList.back();
            m_TextureFreeList.pop_back();
            m_TextureFreeMap[index] = false;
            m_Textures[index] = std::move(textureResult);

            m_PendingResourceOperations.emplace_back(PendingResourceOperation{
                .type = PendingResourceOperation::SlotType::Texture,
                .slot = index,
                .texture = asset});

            return index;
        }

        m_Textures.push_back(std::move(textureResult));
        m_TextureFreeMap[m_Textures.size() - 1] = false;

        m_PendingResourceOperations.emplace_back(PendingResourceOperation{
            .type = PendingResourceOperation::SlotType::Texture,
            .slot = static_cast<uint32_t>(m_Textures.size() - 1),
            .texture = asset});

        return m_Textures.size() - 1;
    }

    uint32_t ResourceBindingRegistry::RegisterSampler(GPUAssetResolveResult samplerResult)
    {
        if (!samplerResult.asset)
        {
            return -1;
        }

        auto* asset = samplerResult.asset;

        std::unique_lock lock(m_SamplerMutex);
        std::unique_lock operationLock(m_PendingResourceOperationMutex);

        if (!m_SamplerFreeList.empty())
        {
            const uint32_t index = m_SamplerFreeList.back();
            m_SamplerFreeList.pop_back();
            m_SamplerFreeMap[index] = false;
            m_Samplers[index] = std::move(samplerResult);

            m_PendingResourceOperations.emplace_back(PendingResourceOperation{
                .type = PendingResourceOperation::SlotType::Sampler,
                .slot = index,
                .sampler = asset});

            return index;
        }

        m_Samplers.push_back(std::move(samplerResult));
        m_SamplerFreeMap[m_Samplers.size() - 1] = false;

        m_PendingResourceOperations.emplace_back(PendingResourceOperation{
            .type = PendingResourceOperation::SlotType::Sampler,
            .slot = static_cast<uint32_t>(m_Samplers.size() - 1),
            .sampler = asset});

        return m_Samplers.size() - 1;
    }

    uint32_t ResourceBindingRegistry::RegisterSamplerWithImage(GPUAssetResolveResult textureResult,
                                                               GPUAssetResolveResult samplerResult)
    {
        if (!textureResult.asset || !samplerResult.asset)
        {
            return -1;
        }

        auto* textureAsset = textureResult.asset;
        auto* samplerAsset = samplerResult.asset;

        std::unique_lock lock(m_CombinedImageSamplerMutex);
        std::unique_lock operationLock(m_PendingResourceOperationMutex);

        if (!m_CombinedImageSamplerFreeList.empty())
        {
            const uint32_t index = m_CombinedImageSamplerFreeList.back();
            m_CombinedImageSamplerFreeList.pop_back();
            m_CombinedImageSamplerFreeMap[index] = false;
            m_CombinedImageSamplers[index] = std::move(std::make_pair(std::move(textureResult),
                                                                      std::move(samplerResult)));

            m_PendingResourceOperations.emplace_back(PendingResourceOperation{
                .type = PendingResourceOperation::SlotType::CombinedImageSampler,
                .slot = index,
                .texture = textureAsset,
                .sampler = samplerAsset});

            return index;
        }

        m_CombinedImageSamplers.emplace_back(std::move(textureResult), std::move(samplerResult));
        m_CombinedImageSamplerFreeMap[m_CombinedImageSamplers.size() - 1] = false;

        m_PendingResourceOperations.emplace_back(PendingResourceOperation{
            .type = PendingResourceOperation::SlotType::CombinedImageSampler,
            .slot = static_cast<uint32_t>(m_CombinedImageSamplers.size() - 1),
            .texture = textureAsset,
            .sampler = samplerAsset});

        return m_CombinedImageSamplers.size() - 1;
    }

    void ResourceBindingRegistry::UnregisterTexture(uint32_t index)
    {
        if (index >= m_Textures.size() || m_TextureFreeMap[index])
        {
            return;
        }
        m_Textures[index] = {nullptr, false};
        m_TextureFreeMap[index] = true;
        m_TextureFreeList.push_back(index);
    }

    void ResourceBindingRegistry::UnregisterSampler(uint32_t index)
    {
        if (index >= m_Samplers.size() || m_SamplerFreeMap[index])
        {
            return;
        }
        m_Samplers[index] = {nullptr, false};
        m_SamplerFreeMap[index] = true;
        m_SamplerFreeList.push_back(index);
    }

    void ResourceBindingRegistry::UnregisterCombinedImageSampler(uint32_t index)
    {
        if (index >= m_CombinedImageSamplers.size() || m_CombinedImageSamplerFreeMap[index])
        {
            return;
        }
        auto combinedResult = std::move(m_CombinedImageSamplers[index]);
        m_CombinedImageSamplers[index] = {nullptr, nullptr};
        m_CombinedImageSamplerFreeMap[index] = true;
        m_CombinedImageSamplerFreeList.push_back(index);
    }

    void ResourceBindingRegistry::UpdateResourceGroupForPendingOperations(RHIResourceGroup* group)
    {
        std::unique_lock operationLock(m_PendingResourceOperationMutex);
        if (m_PendingResourceOperations.empty())
        {
            return;
        }
        auto pendingOperations = m_PendingResourceOperations;
        m_PendingResourceOperations.clear();
        operationLock.unlock();

        for (auto& operation : m_PendingResourceOperations)
        {
            switch (operation.type)
            {
                case PendingResourceOperation::SlotType::Texture:
                {
                    if (operation.texture)
                    {
                        group->WriteImageView(kTextureBindingSlot,
                                              static_cast<GPUTextureAsset*>(operation.texture)->GetDefaultImageView(),
                                              RHIImageResourceState::ShaderRead,
                                              operation.slot);
                    }
                    break;
                }
                case PendingResourceOperation::SlotType::Sampler:
                {
                    if (operation.sampler)
                    {
                        group->WriteSampler(kSamplerBindingSlot,
                                            static_cast<GPUSamplerAsset*>(operation.sampler)->GetHandle(),
                                            operation.slot);
                    }
                    break;
                }
                case PendingResourceOperation::SlotType::CombinedImageSampler:
                {
                    if (operation.texture && operation.sampler)
                    {
                        group->WriteSamplerWithImage(
                            kCombinedImageSamplerBindingSlot,
                            static_cast<GPUSamplerAsset*>(operation.sampler)->GetHandle(),
                            static_cast<GPUTextureAsset*>(operation.texture)->GetDefaultImageView(),
                            RHIImageResourceState::ShaderRead,
                            operation.slot);
                    }
                    break;
                }
            }
        }
    }
} // Hazel