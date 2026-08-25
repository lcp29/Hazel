#pragma once

#include "GPUAsset/CachedMaterial.h"
#include "GPUAsset/GPUGraphicsPipelineAsset.h"
#include "GPUAsset/GPUSamplerAsset.h"
#include "GPUAsset/Importer/GPUAssetImporter.h"
#include "Hazel/Renderer/GPUAsset/GPURenderBufferAsset.h"

#include <thread>
#include <tuple>
#include <type_traits>

namespace Hazel
{
    template <typename> constexpr bool kDependentFalse = false;

    template <typename TAsset, typename... Args> Aster::GPUAssetHandle Renderer::ResolveDirectGPUAsset(Args&&... args)
    {
        if constexpr (std::is_same_v<TAsset, Aster::GPUGraphicsPipelineAsset>)
        {
            auto argsTuple = std::forward_as_tuple(std::forward<Args>(args)...);
            Hazel::UUID material = std::get<0>(argsTuple);
            const auto& colorAttachmentFormats = std::get<1>(argsTuple);
            const auto& colorBlendAttachments = std::get<2>(argsTuple);
            Aster::RHIFormat depthStencilFormat = std::get<3>(argsTuple);

            auto materialResult = ResolveGPUAsset(material, Aster::AssetType::Material);
            auto* materialAsset = static_cast<Aster::CachedMaterial*>(materialResult.asset);
            if (!materialAsset) { return Aster::GPUAssetHandle(nullptr, false); }

            Hazel::UUID pipelineUUID =
                materialAsset->GetPipelineKey(colorAttachmentFormats, colorBlendAttachments, depthStencilFormat);
            Aster::GPUGraphicsPipelineAsset* pipelineAsset = nullptr;
            bool shouldSpawn = false;
            {
                auto& mutex = m_GPUAssetRegistry->GetPipelineCacheMutex();
                std::unique_lock lock(mutex);
                if (m_GPUAssetRegistry->HasAsset(pipelineUUID))
                {
                    pipelineAsset =
                        static_cast<Aster::GPUGraphicsPipelineAsset*>(m_GPUAssetRegistry->GetAsset(pipelineUUID));
                }
                else
                {
                    auto placeholder =
                        std::make_unique<Aster::GPUGraphicsPipelineAsset>(pipelineUUID,
                                                                          Aster::AssetType::GraphicsPipeline,
                                                                          nullptr,
                                                                          materialAsset->GetShader(),
                                                                          this,
                                                                          m_CurrentFrame);
                    auto [_, asset] = m_GPUAssetRegistry->SetAssetAndGetTheOldAndTheNewOnes(std::move(placeholder));
                    pipelineAsset = static_cast<Aster::GPUGraphicsPipelineAsset*>(asset);
                    shouldSpawn = true;
                }
            }

            pipelineAsset->SetLastReferencedFrame(m_CurrentFrame);

            {
                std::unique_lock assetLock(pipelineAsset->GetMutex());
                if (!pipelineAsset->IsLoading() && pipelineAsset->GetPipeline() != nullptr)
                {
                    assetLock.unlock();
                    return Aster::GPUAssetHandle(pipelineAsset);
                }
            }

            if (shouldSpawn)
            {
                std::thread(
                    [this, material, colorAttachmentFormats, colorBlendAttachments, depthStencilFormat, pipelineAsset] {
                        auto materialResult = ResolveGPUAssetBlocked(material, Aster::AssetType::Material);
                        auto materialAsset = static_cast<Aster::CachedMaterial*>(materialResult.asset);
                        if (!materialAsset)
                        {
                            std::unique_lock assetLock(pipelineAsset->GetMutex());
                            pipelineAsset->SetLoading(false);
                            assetLock.unlock();
                            pipelineAsset->GetCondition().notify_all();
                            pipelineAsset->Return();
                            return;
                        }

                        Hazel::UUID shaderUUID = materialAsset->GetShader();
                        auto shaderResult = ResolveGPUAssetBlocked(shaderUUID, Aster::AssetType::Shader);
                        if (!shaderResult.asset)
                        {
                            std::unique_lock assetLock(pipelineAsset->GetMutex());
                            pipelineAsset->SetLoading(false);
                            assetLock.unlock();
                            pipelineAsset->GetCondition().notify_all();
                            pipelineAsset->Return();
                            return;
                        }

                        auto pipeline = CreateGraphicsPipeline(
                            material, colorAttachmentFormats, colorBlendAttachments, depthStencilFormat, this);
                        std::unique_lock assetLock(pipelineAsset->GetMutex());
                        pipelineAsset->SetPipeline(pipeline);
                        pipelineAsset->SetLoading(false);
                        assetLock.unlock();
                        pipelineAsset->GetCondition().notify_all();
                        pipelineAsset->Return();
                    })
                    .detach();
                return Aster::GPUAssetHandle(nullptr, false);
            }

            pipelineAsset->Return();
            return Aster::GPUAssetHandle(nullptr, false);
        }
        else if constexpr (std::is_same_v<TAsset, Aster::GPURenderTextureAsset>)
        {
            auto argsTuple = std::forward_as_tuple(std::forward<Args>(args)...);
            const auto& desc = std::get<0>(argsTuple);
            uint64_t lastReferencedFrame =
                sizeof...(Args) >= 2 ? static_cast<uint64_t>(std::get<1>(argsTuple)) : m_CurrentFrame;
            if (lastReferencedFrame == static_cast<uint64_t>(-1)) { lastReferencedFrame = m_CurrentFrame; }

            auto asset = CreateGPURenderTextureAsset(this, Hazel::UUID(), 0, desc, lastReferencedFrame);
            auto* currentAsset = asset.get();
            if (!currentAsset) { return Aster::GPUAssetHandle(nullptr, false); }

            m_GPUAssetRegistry->AddAsset(std::move(asset));
            currentAsset->Use();
            return Aster::GPUAssetHandle(currentAsset);
        }
        else if constexpr (std::is_same_v<TAsset, Aster::GPURenderBufferAsset>)
        {
            auto argsTuple = std::forward_as_tuple(std::forward<Args>(args)...);
            const auto& desc = std::get<0>(argsTuple);
            uint64_t lastReferencedFrame =
                sizeof...(Args) >= 2 ? static_cast<uint64_t>(std::get<1>(argsTuple)) : m_CurrentFrame;
            if (lastReferencedFrame == static_cast<uint64_t>(-1)) { lastReferencedFrame = m_CurrentFrame; }

            auto asset = CreateGPURenderBufferAsset(this, Hazel::UUID(), 0, desc, lastReferencedFrame);
            auto* currentAsset = asset.get();
            if (!currentAsset) { return Aster::GPUAssetHandle(nullptr, false); }

            m_GPUAssetRegistry->AddAsset(std::move(asset));
            currentAsset->Use();
            return Aster::GPUAssetHandle(currentAsset);
        }
        else if constexpr (std::is_same_v<TAsset, Aster::GPUSamplerAsset>)
        {
            auto argsTuple = std::forward_as_tuple(std::forward<Args>(args)...);
            const auto& desc = std::get<0>(argsTuple);
            uint64_t lastReferencedFrame =
                sizeof...(Args) >= 2 ? static_cast<uint64_t>(std::get<1>(argsTuple)) : m_CurrentFrame;
            if (lastReferencedFrame == static_cast<uint64_t>(-1)) { lastReferencedFrame = m_CurrentFrame; }

            auto sampler = GetDevice()->CreateSampler(desc);
            auto asset =
                std::make_unique<Aster::GPUSamplerAsset>(Hazel::UUID(), 0, this, desc, sampler, lastReferencedFrame);
            auto* currentAsset = asset.get();
            if (!currentAsset) { return Aster::GPUAssetHandle(nullptr, false); }

            m_GPUAssetRegistry->AddAsset(std::move(asset));
            currentAsset->Use();
            return Aster::GPUAssetHandle(currentAsset);
        }
        else
        {
            static_assert(kDependentFalse<TAsset>, "Unsupported direct GPU asset type");
        }
    }

    template <typename TAsset, typename... Args>
    Aster::GPUAssetHandle Renderer::ResolveDirectGPUAssetBlocked(Args&&... args)
    {
        if constexpr (std::is_same_v<TAsset, Aster::GPUGraphicsPipelineAsset>)
        {
            auto argsTuple = std::forward_as_tuple(std::forward<Args>(args)...);
            Hazel::UUID material = std::get<0>(argsTuple);
            const auto& colorAttachmentFormats = std::get<1>(argsTuple);
            const auto& colorBlendAttachments = std::get<2>(argsTuple);
            Aster::RHIFormat depthStencilFormat = std::get<3>(argsTuple);

            auto materialResult = ResolveGPUAssetBlocked(material, Aster::AssetType::Material);
            auto materialAsset = static_cast<Aster::CachedMaterial*>(materialResult.asset);
            if (!materialAsset) { return Aster::GPUAssetHandle(nullptr, false); }

            Hazel::UUID shaderUUID = materialAsset->GetShader();
            auto shaderResult = ResolveGPUAssetBlocked(shaderUUID, Aster::AssetType::Shader);
            if (!shaderResult.asset) { return Aster::GPUAssetHandle(nullptr, false); }

            Hazel::UUID pipelineUUID =
                materialAsset->GetPipelineKey(colorAttachmentFormats, colorBlendAttachments, depthStencilFormat);
            Aster::GPUGraphicsPipelineAsset* pipelineAsset = nullptr;
            bool shouldCreate = false;
            {
                auto& mutex = m_GPUAssetRegistry->GetPipelineCacheMutex();
                std::unique_lock lock(mutex);
                if (m_GPUAssetRegistry->HasAsset(pipelineUUID))
                {
                    pipelineAsset =
                        static_cast<Aster::GPUGraphicsPipelineAsset*>(m_GPUAssetRegistry->GetAsset(pipelineUUID));
                }
                else
                {
                    auto placeholder = std::make_unique<Aster::GPUGraphicsPipelineAsset>(
                        pipelineUUID, Aster::AssetType::GraphicsPipeline, nullptr, shaderUUID, this, m_CurrentFrame);
                    auto [_, asset] = m_GPUAssetRegistry->SetAssetAndGetTheOldAndTheNewOnes(std::move(placeholder));
                    pipelineAsset = static_cast<Aster::GPUGraphicsPipelineAsset*>(asset);
                    shouldCreate = true;
                }
            }

            pipelineAsset->SetLastReferencedFrame(m_CurrentFrame);

            {
                std::unique_lock assetLock(pipelineAsset->GetMutex());
                if (!pipelineAsset->IsLoading() && pipelineAsset->GetPipeline() != nullptr)
                {
                    return Aster::GPUAssetHandle(pipelineAsset);
                }

                if (!shouldCreate && pipelineAsset->IsLoading())
                {
                    pipelineAsset->GetCondition().wait(assetLock,
                                                       [pipelineAsset] { return !pipelineAsset->IsLoading(); });
                    const bool hasPipeline = pipelineAsset->GetPipeline() != nullptr;
                    assetLock.unlock();
                    if (hasPipeline) { return Aster::GPUAssetHandle(pipelineAsset); }
                    pipelineAsset->Return();
                    return Aster::GPUAssetHandle(nullptr, false);
                }
            }

            if (shouldCreate)
            {
                auto pipeline = CreateGraphicsPipeline(
                    material, colorAttachmentFormats, colorBlendAttachments, depthStencilFormat, this);
                std::unique_lock assetLock(pipelineAsset->GetMutex());
                pipelineAsset->SetPipeline(pipeline);
                pipelineAsset->SetLoading(false);
                assetLock.unlock();
                pipelineAsset->GetCondition().notify_all();
                if (pipeline) { return Aster::GPUAssetHandle(pipelineAsset); }
                pipelineAsset->Return();
                return Aster::GPUAssetHandle(nullptr, false);
            }

            pipelineAsset->Return();
            return Aster::GPUAssetHandle(nullptr, false);
        }
        else
        {
            return ResolveDirectGPUAsset<TAsset>(std::forward<Args>(args)...);
        }
    }
} // namespace Hazel
