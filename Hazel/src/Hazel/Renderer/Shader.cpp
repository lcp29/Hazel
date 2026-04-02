//
// Created by helmholtz on 2026/3/31.
//

#include "Shader.h"

#include "Hazel/Asset/MaterialAsset.h"
#include "Hazel/Renderer/RenderBuffer.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/ResourceHeapAllocator.h"
#include "Hazel/Renderer/Sampler.h"
#include "Hazel/Renderer/Texture.h"

#include <algorithm>
#include <cstring>
#include <ranges>

namespace Hazel
{
    namespace
    {
        constexpr uint32_t kMaterialResourceSet = 2;

        struct ReflectedSetData
        {
            RHIResourceLayoutDesc layoutDesc{};
        };

        uint32_t GetMaterialCapacity(const std::vector<std::unique_ptr<Material>>& materials)
        {
            uint32_t maxMaterialID = 0;
            bool hasMaterial = false;
            for (uint32_t i = 0; i < materials.size(); i++)
            {
                if (!materials[i])
                {
                    continue;
                }
                maxMaterialID = i;
                hasMaterial = true;
            }
            return hasMaterial ? maxMaterialID + 1 : 1;
        }

        uint32_t GetMaterialBufferStride(const RHIShaderReflection& reflection)
        {
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
                        return slot.buffer.size;
                    }
                }
            }
            return 0;
        }

        RHIResourceBindingType FindMaterialSlotType(const RHIShaderReflection& reflection, uint32_t slotNumber)
        {
            for (const auto& group : reflection.resourceGroups)
            {
                if (group.set != kMaterialResourceSet)
                {
                    continue;
                }
                for (const auto& slot : group.slots)
                {
                    if (slot.slot == slotNumber)
                    {
                        return slot.type;
                    }
                }
            }
            return RHIResourceBindingType::UniformBuffer;
        }

        void AddReflectionToSetData(std::vector<ReflectedSetData>& setData,
                                    const RHIShaderReflection& reflection,
                                    RHIShaderStages stage,
                                    uint32_t materialCapacity)
        {
            for (const auto& group : reflection.resourceGroups)
            {
                if (group.set >= setData.size())
                {
                    setData.resize(group.set + 1);
                }

                auto& layoutDesc = setData[group.set].layoutDesc;
                for (const auto& slot : group.slots)
                {
                    auto bindingIt = std::ranges::find_if(layoutDesc.bindings,
                                                          [&slot](const RHIResourceBindingSlotDesc& binding) {
                                                              return binding.slot == slot.slot;
                                                          });

                    uint32_t bindingCount = slot.count;
                    if (group.set == kMaterialResourceSet)
                    {
                        bindingCount = slot.slot == 0 ? 1 : materialCapacity;
                    }

                    if (bindingIt == layoutDesc.bindings.end())
                    {
                        RHIResourceBindingSlotDesc bindingDesc{};
                        bindingDesc.slot = slot.slot;
                        bindingDesc.type = slot.type;
                        bindingDesc.count = bindingCount;
                        bindingDesc.stages = stage;
                        layoutDesc.bindings.push_back(bindingDesc);
                    }
                    else
                    {
                        bindingIt->stages |= stage;
                        bindingIt->count = bindingCount;
                    }
                }
            }
        }

        std::vector<RHIPushConstantRangeDesc> BuildPushConstantRanges(const RHIShaderReflection& vertexReflection,
                                                                      const RHIShaderReflection& fragmentReflection)
        {
            std::vector<RHIPushConstantRangeDesc> ranges;

            auto appendRanges = [&ranges](const RHIShaderReflection& reflection, RHIShaderStages stages) {
                for (const auto& pushConstant : reflection.pushConstants)
                {
                    auto it = std::ranges::find_if(ranges,
                                                   [&pushConstant](const RHIPushConstantRangeDesc& range) {
                                                       return range.offset == pushConstant.offset && range.size ==
                                                              pushConstant.
                                                              size;
                                                   });
                    if (it == ranges.end())
                    {
                        RHIPushConstantRangeDesc range{};
                        range.offset = pushConstant.offset;
                        range.size = pushConstant.size;
                        range.stages = stages;
                        ranges.push_back(range);
                    }
                    else
                    {
                        it->stages |= stages;
                    }
                }
            };

            appendRanges(vertexReflection, RHIShaderStageFlagBits::Vertex);
            appendRanges(fragmentReflection, RHIShaderStageFlagBits::Fragment);
            return ranges;
        }
    } // namespace

    Shader::Shader(UUID uuid, Renderer* renderer, RHIShader* vertexShader, RHIShader* fragmentShader)
        : m_IsValid(true), m_UUID(uuid), m_Renderer(renderer), m_VertexShader(vertexShader),
          m_FragmentShader(fragmentShader)
    {
        RecreateMaterialResourceGroup();
    }

    Shader::Shader(Shader&& other) noexcept
        : m_IsValid(other.m_IsValid),
          m_UUID(other.m_UUID),
          m_Renderer(other.m_Renderer),
          m_VertexShader(other.m_VertexShader),
          m_FragmentShader(other.m_FragmentShader),
          m_Materials(std::move(other.m_Materials)),
          m_MaterialListFreeList(std::move(other.m_MaterialListFreeList)),
          m_ResourceLayouts(std::move(other.m_ResourceLayouts)),
          m_ResourceSignature(other.m_ResourceSignature),
          m_MaterialResourceLayout(other.m_MaterialResourceLayout),
          m_MaterialResourceGroup(other.m_MaterialResourceGroup),
          m_MaterialResourceHeap(other.m_MaterialResourceHeap),
          m_MaterialBuffer(other.m_MaterialBuffer),
          m_MaterialCapacity(other.m_MaterialCapacity),
          m_MaterialBufferStride(other.m_MaterialBufferStride)
    {
        other.m_IsValid = false;
        other.m_Renderer = nullptr;
        other.m_VertexShader = nullptr;
        other.m_FragmentShader = nullptr;
        other.m_ResourceSignature = nullptr;
        other.m_MaterialResourceLayout = nullptr;
        other.m_MaterialResourceGroup = nullptr;
        other.m_MaterialResourceHeap = nullptr;
        other.m_MaterialBuffer = nullptr;
        other.m_MaterialCapacity = 0;
        other.m_MaterialBufferStride = 0;
    }

    Shader& Shader::operator=(Shader&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Release();

        m_IsValid = other.m_IsValid;
        m_UUID = other.m_UUID;
        m_Renderer = other.m_Renderer;
        m_VertexShader = other.m_VertexShader;
        m_FragmentShader = other.m_FragmentShader;
        m_Materials = std::move(other.m_Materials);
        m_MaterialListFreeList = std::move(other.m_MaterialListFreeList);
        m_ResourceLayouts = std::move(other.m_ResourceLayouts);
        m_ResourceSignature = other.m_ResourceSignature;
        m_MaterialResourceLayout = other.m_MaterialResourceLayout;
        m_MaterialResourceGroup = other.m_MaterialResourceGroup;
        m_MaterialResourceHeap = other.m_MaterialResourceHeap;
        m_MaterialBuffer = other.m_MaterialBuffer;
        m_MaterialCapacity = other.m_MaterialCapacity;
        m_MaterialBufferStride = other.m_MaterialBufferStride;

        other.m_IsValid = false;
        other.m_Renderer = nullptr;
        other.m_VertexShader = nullptr;
        other.m_FragmentShader = nullptr;
        other.m_ResourceSignature = nullptr;
        other.m_MaterialResourceLayout = nullptr;
        other.m_MaterialResourceGroup = nullptr;
        other.m_MaterialResourceHeap = nullptr;
        other.m_MaterialBuffer = nullptr;
        other.m_MaterialCapacity = 0;
        other.m_MaterialBufferStride = 0;
        return *this;
    }

    Shader::~Shader()
    {
        Release();
    }

    uint32_t Shader::RegisterMaterial(std::unique_ptr<Material> material)
    {
        uint32_t materialID;
        if (!m_MaterialListFreeList.empty())
        {
            materialID = m_MaterialListFreeList.back();
            m_MaterialListFreeList.pop_back();
            material->SetMaterialID(materialID);
            m_Materials[materialID] = std::move(material);
        }
        else
        {
            materialID = m_Materials.size();
            material->SetMaterialID(materialID);
            m_Materials.push_back(std::move(material));
        }
        RecreateMaterialResourceGroup();
        return materialID;
    }

    void Shader::UnregisterMaterial(uint32_t materialID)
    {
        m_Materials[materialID].reset();
        m_MaterialListFreeList.push_back(materialID);
        if (m_IsValid)
        {
            RecreateMaterialResourceGroup();
        }
    }

    void Shader::RecreateMaterialResourceGroup()
    {
        const auto materialCapacity = GetMaterialCapacity(m_Materials);
        const auto materialBufferStride = GetMaterialBufferStride(m_VertexShader->GetReflection());
        const bool recreateResources = m_ResourceSignature == nullptr ||
                                       m_MaterialResourceGroup == nullptr ||
                                       m_MaterialBuffer == nullptr ||
                                       m_MaterialCapacity != materialCapacity ||
                                       m_MaterialBufferStride != materialBufferStride;

        if (!recreateResources)
        {
            WriteMaterialResourceGroup();
            return;
        }

        if (m_MaterialResourceGroup)
        {
            m_Renderer->GetResourceHeapAllocator()->FreeGroup(m_MaterialResourceGroup);
            m_MaterialResourceGroup = nullptr;
            m_MaterialResourceHeap = nullptr;
        }

        if (m_MaterialBuffer)
        {
            m_Renderer->RemoveRenderBuffer(m_MaterialBuffer);
            m_MaterialBuffer = nullptr;
        }

        if (m_ResourceSignature)
        {
            m_ResourceSignature->Release();
            m_ResourceSignature = nullptr;
        }

        for (auto* resourceLayout : m_ResourceLayouts)
        {
            resourceLayout->Release();
        }
        m_ResourceLayouts.clear();
        m_MaterialResourceLayout = nullptr;

        const auto& vertexReflection = m_VertexShader->GetReflection();
        const auto& fragmentReflection = m_FragmentShader->GetReflection();

        std::vector<ReflectedSetData> setData;
        AddReflectionToSetData(setData, vertexReflection, RHIShaderStageFlagBits::Vertex, materialCapacity);
        AddReflectionToSetData(setData, fragmentReflection, RHIShaderStageFlagBits::Fragment, materialCapacity);

        m_ResourceLayouts.reserve(setData.size());
        for (uint32_t set = 0; set < setData.size(); set++)
        {
            std::ranges::sort(setData[set].layoutDesc.bindings,
                              [](const RHIResourceBindingSlotDesc& lhs, const RHIResourceBindingSlotDesc& rhs) {
                                  return lhs.slot < rhs.slot;
                              });
            auto* layout = m_Renderer->GetDevice()->CreateResourceLayout(setData[set].layoutDesc);
            m_ResourceLayouts.push_back(layout);
            if (set == kMaterialResourceSet)
            {
                m_MaterialResourceLayout = layout;
            }
        }

        RHIResourceSignatureDesc signatureDesc{};
        signatureDesc.resourceLayouts = m_ResourceLayouts;
        signatureDesc.pushConstantRanges = BuildPushConstantRanges(vertexReflection, fragmentReflection);
        m_ResourceSignature = m_Renderer->GetDevice()->CreateResourceSignature(signatureDesc);

        RenderBufferDesc bufferDesc{};
        bufferDesc.perFrame = false;
        bufferDesc.size = static_cast<uint64_t>(materialCapacity) * materialBufferStride;
        bufferDesc.usages = RHIBufferUsageFlagBits::UniformBuffer;
        bufferDesc.cpuAccess = RHIBufferCpuAccess::Write;
        bufferDesc.mapOnCreate = true;
        bufferDesc.hostCoherent = true;
        m_MaterialBuffer = m_Renderer->AddRenderBuffer(std::make_unique<RenderBuffer>(m_Renderer, bufferDesc));

        m_MaterialResourceGroup = m_Renderer->GetResourceHeapAllocator()->AllocateGroup(
            m_MaterialResourceLayout,
            &m_MaterialResourceHeap);

        m_MaterialCapacity = materialCapacity;
        m_MaterialBufferStride = materialBufferStride;

        WriteMaterialResourceGroup();
    }

    void Shader::WriteMaterialResourceGroup()
    {
        auto* buffer = m_MaterialBuffer->GetBuffer();
        auto* mappedData = static_cast<uint8_t*>(buffer->Map());
        std::memset(mappedData, 0, static_cast<size_t>(m_MaterialCapacity) * m_MaterialBufferStride);

        for (uint32_t materialID = 0; materialID < m_Materials.size(); materialID++)
        {
            auto& material = m_Materials[materialID];
            if (!material)
            {
                continue;
            }

            const auto baseOffset = static_cast<size_t>(materialID) * m_MaterialBufferStride;
            for (const auto& property : material->GetProperties() | std::views::values)
            {
                if (property.isInBuffer)
                {
                    std::memcpy(mappedData + baseOffset + property.member.offset, property.data, property.member.size);
                    continue;
                }

                const auto slotType = FindMaterialSlotType(m_VertexShader->GetReflection(), property.slot);
                switch (slotType)
                {
                    case RHIResourceBindingType::Sampler:
                        m_MaterialResourceGroup->WriteSampler(
                            property.slot,
                            property.sampler
                                ? property.sampler->GetHandle()
                                : m_Renderer->GetDefaultSampler()->GetHandle(),
                            materialID);
                        break;
                    case RHIResourceBindingType::SampledImage:
                        m_MaterialResourceGroup->WriteImageView(
                            property.slot,
                            property.texture
                                ? property.texture->GetImageView()
                                : m_Renderer->GetWhiteTexture()->GetImageView(),
                            RHIImageResourceState::ShaderRead,
                            materialID);
                        break;
                    case RHIResourceBindingType::SamplerWithImage:
                        m_MaterialResourceGroup->WriteSamplerWithImage(
                            property.slot,
                            property.sampler
                                ? property.sampler->GetHandle()
                                : m_Renderer->GetDefaultSampler()->GetHandle(),
                            property.texture
                                ? property.texture->GetImageView()
                                : m_Renderer->GetWhiteTexture()->GetImageView(),
                            RHIImageResourceState::ShaderRead,
                            materialID);
                        break;
                    default:
                        break;
                }
            }
        }

        buffer->Unmap();
        m_MaterialResourceGroup->WriteBuffer(0,
                                             buffer,
                                             0,
                                             static_cast<uint64_t>(m_MaterialCapacity) * m_MaterialBufferStride,
                                             0);

        auto* cmd = m_Renderer->GetGraphicsContext()->GetDefaultCommandBuffer();
        cmd->Reset();
        cmd->Begin(true);
        cmd->End();

        RHIQueueSubmitDesc submitDesc{};
        submitDesc.commandBuffers = {cmd};
        m_Renderer->GetDevice()->GetUniformQueue()->Submit(submitDesc);
        m_Renderer->GetDevice()->WaitIdle();
    }

    void Shader::Release()
    {
        if (!m_IsValid)
        {
            return;
        }

        m_IsValid = false;

        if (m_MaterialResourceGroup)
        {
            m_Renderer->GetResourceHeapAllocator()->FreeGroup(m_MaterialResourceGroup);
            m_MaterialResourceGroup = nullptr;
            m_MaterialResourceHeap = nullptr;
        }

        if (m_MaterialBuffer)
        {
            m_Renderer->RemoveRenderBuffer(m_MaterialBuffer);
            m_MaterialBuffer = nullptr;
        }

        if (m_ResourceSignature)
        {
            m_ResourceSignature->Release();
            m_ResourceSignature = nullptr;
        }

        for (auto* resourceLayout : m_ResourceLayouts)
        {
            resourceLayout->Release();
        }
        m_ResourceLayouts.clear();
        m_MaterialResourceLayout = nullptr;
        m_MaterialCapacity = 0;
        m_MaterialBufferStride = 0;

        m_VertexShader->Release();
        m_FragmentShader->Release();
        m_VertexShader = nullptr;
        m_FragmentShader = nullptr;
    }

    void Shader::ReleaseImmediate()
    {
        if (!m_IsValid)
        {
            return;
        }

        m_IsValid = false;

        if (m_MaterialResourceGroup)
        {
            m_Renderer->GetResourceHeapAllocator()->FreeGroup(m_MaterialResourceGroup);
            m_MaterialResourceGroup = nullptr;
            m_MaterialResourceHeap = nullptr;
        }

        if (m_MaterialBuffer)
        {
            m_Renderer->RemoveRenderBuffer(m_MaterialBuffer);
            m_MaterialBuffer = nullptr;
        }

        if (m_ResourceSignature)
        {
            m_ResourceSignature->ReleaseImmediate();
            m_ResourceSignature = nullptr;
        }

        for (auto* resourceLayout : m_ResourceLayouts)
        {
            resourceLayout->ReleaseImmediate();
        }
        m_ResourceLayouts.clear();
        m_MaterialResourceLayout = nullptr;
        m_MaterialCapacity = 0;
        m_MaterialBufferStride = 0;

        m_VertexShader->ReleaseImmediate();
        m_FragmentShader->ReleaseImmediate();
        m_VertexShader = nullptr;
        m_FragmentShader = nullptr;
    }
} // namespace Hazel