//
// Created by helmholtz on 2026/4/5.
//

#include "MaterialShaderRegistry.h"
#include "GPUAsset/GPUSamplerAsset.h"
#include "GPUAsset/GPUShaderAsset.h"
#include "GPUAsset/GPUTextureAsset.h"
#include "GPUAsset/Importer/GPUAssetImporterInternal.h"
#include "Renderer.h"
#include "ResourceHeapAllocator.h"
#include "Hazel/RHI/RHI.h"

#include <algorithm>

namespace Hazel
{
    constexpr uint32_t kMaterialDescriptorMaxCount = 512;

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
        GPUAssetImporterInternal::AddReflectionToSetData(
            setData,
            reflection,
            RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment);

        RHIResourceLayoutDesc materialPropertiesLayoutDesc{};
        materialPropertiesLayoutDesc.bindings.emplace_back(RHIResourceBindingSlotDesc{
            .slot = 0,
            .type = RHIResourceBindingType::StorageBuffer,
            .count = 1,
            .stages = RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment,});

        setData[GPUAssetImporterInternal::kMaterialResourceSet] = materialPropertiesLayoutDesc;

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
            if (set == GPUAssetImporterInternal::kMaterialResourceSet)
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
            GPUAssetImporterInternal::AddReflectionToSetData(
                setData,
                reflection,
                RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment);

            for (const auto& group : reflection.resourceGroups)
            {
                if (group.set != GPUAssetImporterInternal::kMaterialResourceSet)
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
}