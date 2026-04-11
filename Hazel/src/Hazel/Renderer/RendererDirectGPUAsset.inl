#pragma once

#include "GPUAsset/GPUGraphicsPipelineAsset.h"
#include "GPUAsset/GPUSamplerAsset.h"
#include "GPUAsset/Importer/GPUAssetImporter.h"
#include "Hazel/Renderer/GPUAsset/GPURenderBufferAsset.h"
#include "GPUAsset/CachedMaterial.h"

#include <thread>
#include <tuple>
#include <type_traits>

namespace Hazel
{
    template <typename>
    constexpr bool kDependentFalse = false;

    template <typename TAsset, typename... Args>
    GPUAssetHandle Renderer::ResolveDirectGPUAsset(Args&&... args)
    {
        if constexpr (std::is_same_v<TAsset, GPUGraphicsPipelineAsset>)
        {
            auto argsTuple = std::forward_as_tuple(std::forward<Args>(args)...);
            UUID material = std::get<0>(argsTuple);
            const auto& colorAttachmentFormats = std::get<1>(argsTuple);
            const auto& colorBlendAttachments = std::get<2>(argsTuple);
            RHIFormat depthStencilFormat = std::get<3>(argsTuple);

            auto materialResult = ResolveGPUAsset(material, AssetType::Material);
            auto* materialAsset = static_cast<CachedMaterial*>(materialResult.asset);
            if (!materialAsset)
            {
                return GPUAssetHandle(nullptr, false);
            }

            UUID pipelineUUID = materialAsset->GetPipelineKey(colorAttachmentFormats,
                                                              colorBlendAttachments,
                                                              depthStencilFormat);
            GPUGraphicsPipelineAsset* pipelineAsset = nullptr;
            bool shouldSpawn = false;
            {
                auto& mutex = m_GPUAssetRegistry->GetPipelineCacheMutex();
                std::unique_lock lock(mutex);
                if (m_GPUAssetRegistry->HasAsset(pipelineUUID))
                {
                    pipelineAsset = static_cast<GPUGraphicsPipelineAsset*>(m_GPUAssetRegistry->GetAsset(pipelineUUID));
                }
                else
                {
                    auto placeholder = std::make_unique<GPUGraphicsPipelineAsset>(pipelineUUID,
                        AssetType::GraphicsPipeline,
                        nullptr,
                        materialAsset->GetShader(),
                        this,
                        m_CurrentFrame);
                    auto [_, asset] = m_GPUAssetRegistry->SetAssetAndGetTheOldAndTheNewOnes(std::move(placeholder));
                    pipelineAsset = static_cast<GPUGraphicsPipelineAsset*>(asset);
                    shouldSpawn = true;
                }
            }

            pipelineAsset->SetLastReferencedFrame(m_CurrentFrame);

            {
                std::unique_lock assetLock(pipelineAsset->GetMutex());
                if (!pipelineAsset->IsLoading() && pipelineAsset->GetPipeline() != nullptr)
                {
                    assetLock.unlock();
                    return GPUAssetHandle(pipelineAsset);
                }
            }

            if (shouldSpawn)
            {
                std::thread([this, material, colorAttachmentFormats, colorBlendAttachments, depthStencilFormat, pipelineAsset] {
                    auto materialResult = ResolveGPUAssetBlocked(material, AssetType::Material);
                    auto materialAsset = static_cast<CachedMaterial*>(materialResult.asset);
                    if (!materialAsset)
                    {
                        std::unique_lock assetLock(pipelineAsset->GetMutex());
                        pipelineAsset->SetLoading(false);
                        assetLock.unlock();
                        pipelineAsset->GetCondition().notify_all();
                        pipelineAsset->Return();
                        return;
                    }

                    UUID shaderUUID = materialAsset->GetShader();
                    auto shaderResult = ResolveGPUAssetBlocked(shaderUUID, AssetType::Shader);
                    if (!shaderResult.asset)
                    {
                        std::unique_lock assetLock(pipelineAsset->GetMutex());
                        pipelineAsset->SetLoading(false);
                        assetLock.unlock();
                        pipelineAsset->GetCondition().notify_all();
                        pipelineAsset->Return();
                        return;
                    }

                    auto pipeline = CreateGraphicsPipeline(material,
                                                           colorAttachmentFormats,
                                                           colorBlendAttachments,
                                                           depthStencilFormat,
                                                           this);
                    std::unique_lock assetLock(pipelineAsset->GetMutex());
                    pipelineAsset->SetPipeline(pipeline);
                    pipelineAsset->SetLoading(false);
                    assetLock.unlock();
                    pipelineAsset->GetCondition().notify_all();
                    pipelineAsset->Return();
                }).detach();
                return GPUAssetHandle(nullptr, false);
            }

            pipelineAsset->Return();
            return GPUAssetHandle(nullptr, false);
        }
        else if constexpr (std::is_same_v<TAsset, GPURenderTextureAsset>)
        {
            auto argsTuple = std::forward_as_tuple(std::forward<Args>(args)...);
            const auto& desc = std::get<0>(argsTuple);
            uint64_t lastReferencedFrame = sizeof...(Args) >= 2
                                               ? static_cast<uint64_t>(std::get<1>(argsTuple))
                                               : m_CurrentFrame;
            if (lastReferencedFrame == static_cast<uint64_t>(-1))
            {
                lastReferencedFrame = m_CurrentFrame;
            }

            auto asset = CreateGPURenderTextureAsset(this, UUID(), 0, desc, lastReferencedFrame);
            auto* currentAsset = asset.get();
            if (!currentAsset)
            {
                return GPUAssetHandle(nullptr, false);
            }

            m_GPUAssetRegistry->AddAsset(std::move(asset));
            currentAsset->Use();
            return GPUAssetHandle(currentAsset);
        }
        else if constexpr (std::is_same_v<TAsset, GPURenderBufferAsset>)
        {
            auto argsTuple = std::forward_as_tuple(std::forward<Args>(args)...);
            const auto& desc = std::get<0>(argsTuple);
            uint64_t lastReferencedFrame = sizeof...(Args) >= 2
                                               ? static_cast<uint64_t>(std::get<1>(argsTuple))
                                               : m_CurrentFrame;
            if (lastReferencedFrame == static_cast<uint64_t>(-1))
            {
                lastReferencedFrame = m_CurrentFrame;
            }

            auto asset = CreateGPURenderBufferAsset(this, UUID(), 0, desc, lastReferencedFrame);
            auto* currentAsset = asset.get();
            if (!currentAsset)
            {
                return GPUAssetHandle(nullptr, false);
            }

            m_GPUAssetRegistry->AddAsset(std::move(asset));
            currentAsset->Use();
            return GPUAssetHandle(currentAsset);
        }
        else if constexpr (std::is_same_v<TAsset, GPUSamplerAsset>)
        {
            auto argsTuple = std::forward_as_tuple(std::forward<Args>(args)...);
            const auto& desc = std::get<0>(argsTuple);
            uint64_t lastReferencedFrame = sizeof...(Args) >= 2
                                               ? static_cast<uint64_t>(std::get<1>(argsTuple))
                                               : m_CurrentFrame;
            if (lastReferencedFrame == static_cast<uint64_t>(-1))
            {
                lastReferencedFrame = m_CurrentFrame;
            }

            auto sampler = GetDevice()->CreateSampler(desc);
            auto asset = std::make_unique<GPUSamplerAsset>(UUID(),
                                                           0,
                                                           this,
                                                           desc,
                                                           sampler,
                                                           lastReferencedFrame);
            auto* currentAsset = asset.get();
            if (!currentAsset)
            {
                return GPUAssetHandle(nullptr, false);
            }

            m_GPUAssetRegistry->AddAsset(std::move(asset));
            currentAsset->Use();
            return GPUAssetHandle(currentAsset);
        }
        else
        {
            static_assert(kDependentFalse<TAsset>, "Unsupported direct GPU asset type");
        }
    }

    template <typename TAsset, typename... Args>
    GPUAssetHandle Renderer::ResolveDirectGPUAssetBlocked(Args&&... args)
    {
        if constexpr (std::is_same_v<TAsset, GPUGraphicsPipelineAsset>)
        {
            auto argsTuple = std::forward_as_tuple(std::forward<Args>(args)...);
            UUID material = std::get<0>(argsTuple);
            const auto& colorAttachmentFormats = std::get<1>(argsTuple);
            const auto& colorBlendAttachments = std::get<2>(argsTuple);
            RHIFormat depthStencilFormat = std::get<3>(argsTuple);

            auto materialResult = ResolveGPUAssetBlocked(material, AssetType::Material);
            auto materialAsset = static_cast<CachedMaterial*>(materialResult.asset);
            if (!materialAsset)
            {
                return GPUAssetHandle(nullptr, false);
            }

            UUID shaderUUID = materialAsset->GetShader();
            auto shaderResult = ResolveGPUAssetBlocked(shaderUUID, AssetType::Shader);
            if (!shaderResult.asset)
            {
                return GPUAssetHandle(nullptr, false);
            }

            UUID pipelineUUID = materialAsset->GetPipelineKey(colorAttachmentFormats,
                                                              colorBlendAttachments,
                                                              depthStencilFormat);
            GPUGraphicsPipelineAsset* pipelineAsset = nullptr;
            bool shouldCreate = false;
            {
                auto& mutex = m_GPUAssetRegistry->GetPipelineCacheMutex();
                std::unique_lock lock(mutex);
                if (m_GPUAssetRegistry->HasAsset(pipelineUUID))
                {
                    pipelineAsset = static_cast<GPUGraphicsPipelineAsset*>(m_GPUAssetRegistry->GetAsset(pipelineUUID));
                }
                else
                {
                    auto placeholder = std::make_unique<GPUGraphicsPipelineAsset>(pipelineUUID,
                        AssetType::GraphicsPipeline,
                        nullptr,
                        shaderUUID,
                        this,
                        m_CurrentFrame);
                    auto [_, asset] = m_GPUAssetRegistry->SetAssetAndGetTheOldAndTheNewOnes(std::move(placeholder));
                    pipelineAsset = static_cast<GPUGraphicsPipelineAsset*>(asset);
                    shouldCreate = true;
                }
            }

            pipelineAsset->SetLastReferencedFrame(m_CurrentFrame);

            {
                std::unique_lock assetLock(pipelineAsset->GetMutex());
                if (!pipelineAsset->IsLoading() && pipelineAsset->GetPipeline() != nullptr)
                {
                    return GPUAssetHandle(pipelineAsset);
                }

                if (!shouldCreate && pipelineAsset->IsLoading())
                {
                    pipelineAsset->GetCondition().wait(assetLock,
                                                       [pipelineAsset] {
                                                           return !pipelineAsset->IsLoading();
                                                       });
                    const bool hasPipeline = pipelineAsset->GetPipeline() != nullptr;
                    assetLock.unlock();
                    if (hasPipeline)
                    {
                        return GPUAssetHandle(pipelineAsset);
                    }
                    pipelineAsset->Return();
                    return GPUAssetHandle(nullptr, false);
                }
            }

            if (shouldCreate)
            {
                auto pipeline = CreateGraphicsPipeline(material,
                                                       colorAttachmentFormats,
                                                       colorBlendAttachments,
                                                       depthStencilFormat,
                                                       this);
                std::unique_lock assetLock(pipelineAsset->GetMutex());
                pipelineAsset->SetPipeline(pipeline);
                pipelineAsset->SetLoading(false);
                assetLock.unlock();
                pipelineAsset->GetCondition().notify_all();
                if (pipeline)
                {
                    return GPUAssetHandle(pipelineAsset);
                }
                pipelineAsset->Return();
                return GPUAssetHandle(nullptr, false);
            }

            pipelineAsset->Return();
            return GPUAssetHandle(nullptr, false);
        }
        else
        {
            return ResolveDirectGPUAsset<TAsset>(std::forward<Args>(args)...);
        }
    }
} // namespace Hazel
