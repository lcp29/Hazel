//
// Created by helmholtz on 2026/3/25.
//

#pragma once

#include "Asset.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"
#include "Hazel/Renderer/GPUAsset/GPUSamplerAsset.h"

#include <filesystem>
#include <optional>
#include <yaml-cpp/yaml.h>

namespace Hazel
{
    class Renderer;

    struct SamplerAssetMeta
    {
        YAML::Node Serialize() const;
        static SamplerAssetMeta Deserialize(const YAML::Node& node);
        static SamplerAssetMeta CreateDefault();

        UUID GetUUID() const { return m_UUID; }

        uint64_t GetVersion() const { return m_Version; }

        void VersionUp() { m_Version++; }

        const RHISamplerDesc& GetDesc() const { return m_Desc; }

        void SetMinFilter(RHISamplerFilter minFilter)
        {
            if (m_Desc.minFilter == minFilter) { return; }
            m_Desc.minFilter = minFilter;
            VersionUp();
        }

        void SetMagFilter(RHISamplerFilter magFilter)
        {
            if (m_Desc.magFilter == magFilter) { return; }
            m_Desc.magFilter = magFilter;
            VersionUp();
        }

        void SetMipFilter(RHISamplerFilter mipFilter)
        {
            if (m_Desc.mipFilter == mipFilter) { return; }
            m_Desc.mipFilter = mipFilter;
            VersionUp();
        }

        void SetAddressModeU(RHISamplerAddressMode addressModeU)
        {
            if (m_Desc.addressModeU == addressModeU) { return; }
            m_Desc.addressModeU = addressModeU;
            VersionUp();
        }

        void SetAddressModeV(RHISamplerAddressMode addressModeV)
        {
            if (m_Desc.addressModeV == addressModeV) { return; }
            m_Desc.addressModeV = addressModeV;
            VersionUp();
        }

        void SetAddressModeW(RHISamplerAddressMode addressModeW)
        {
            if (m_Desc.addressModeW == addressModeW) { return; }
            m_Desc.addressModeW = addressModeW;
            VersionUp();
        }

        void SetMipLodBias(float mipLodBias)
        {
            if (m_Desc.mipLodBias == mipLodBias) { return; }
            m_Desc.mipLodBias = mipLodBias;
            VersionUp();
        }

        void SetMinLod(float minLod)
        {
            if (m_Desc.minLod == minLod) { return; }
            m_Desc.minLod = minLod;
            VersionUp();
        }

        void SetMaxLod(float maxLod)
        {
            if (m_Desc.maxLod == maxLod) { return; }
            m_Desc.maxLod = maxLod;
            VersionUp();
        }

        void SetMaxAnisotropy(float maxAnisotropy)
        {
            if (m_Desc.maxAnisotropy == maxAnisotropy) { return; }
            m_Desc.maxAnisotropy = maxAnisotropy;
            VersionUp();
        }

        void SetEnableAnisotropy(bool enableAnisotropy)
        {
            if (m_Desc.enableAnisotropy == enableAnisotropy) { return; }
            m_Desc.enableAnisotropy = enableAnisotropy;
            VersionUp();
        }

        void SetCompareEnable(bool compareEnable)
        {
            if (m_Desc.compareEnable == compareEnable) { return; }
            m_Desc.compareEnable = compareEnable;
            VersionUp();
        }

        void SetCompareOp(RHICompareOp compareOp)
        {
            if (m_Desc.compareOp == compareOp) { return; }
            m_Desc.compareOp = compareOp;
            VersionUp();
        }

      private:
        UUID m_UUID = 0;
        uint64_t m_Version = 0;
        RHISamplerDesc m_Desc{};
    };

    class SamplerAsset : public Asset
    {
      public:
        SamplerAsset(AssetRegistryTerm* registryTerm, SamplerAssetMeta meta)
            : Asset(registryTerm)
            , m_Meta(std::move(meta))
        {}

        uint64_t GetVersion() const final { return m_Meta.GetVersion(); }

        void VersionUp() final { m_Meta.VersionUp(); }

        const SamplerAssetMeta& GetMeta() const { return m_Meta; }

        SamplerAssetMeta& GetMeta() { return m_Meta; }

      private:
        SamplerAssetMeta m_Meta{};
    };
} // namespace Hazel