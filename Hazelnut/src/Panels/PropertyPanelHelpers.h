#pragma once

#include "Hazel/Asset/Asset.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Aster::PropertyPanelHelpers
{
    void
    DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f);

    const char* GetFormatName(RHIFormat format);
    bool DrawFormatCombo(const char* label, RHIFormat& format);

    const char* GetPolygonModeName(RHIPolygonMode polygonMode);
    bool DrawPolygonModeCombo(const char* label, RHIPolygonMode& polygonMode);

    const char* GetCullModeName(RHICullMode cullMode);
    bool DrawCullModeCombo(const char* label, RHICullMode& cullMode);

    const char* GetViewTypeName(RHIImageViewType viewType);
    bool DrawViewTypeCombo(const char* label, RHIImageViewType& viewType);

    const char* GetFilterName(RHISamplerFilter filter);
    bool DrawFilterCombo(const char* label, RHISamplerFilter& filter);

    const char* GetAddressModeName(RHISamplerAddressMode addressMode);
    bool DrawAddressModeCombo(const char* label, RHISamplerAddressMode& addressMode);

    const char* GetCompareOpName(RHICompareOp compareOp);
    bool DrawCompareOpCombo(const char* label, RHICompareOp& compareOp);

    bool DrawImageUsageCheckbox(const char* label, RHIImageUsages& usages, RHIImageUsageFlagBits flag);

    std::string GetAssetLabel(const std::vector<AssetRegistryTerm*>& assets, Hazel::UUID uuid);
    bool DrawAssetRegistryCombo(const char* label,
                                const std::vector<AssetRegistryTerm*>& assets,
                                Hazel::UUID& value,
                                bool allowNone = true);
} // namespace Aster::PropertyPanelHelpers
