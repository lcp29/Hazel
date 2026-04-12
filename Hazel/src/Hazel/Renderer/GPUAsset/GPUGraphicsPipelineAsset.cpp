//
// Created by helmholtz on 2026/4/5.
//

#include "GPUGraphicsPipelineAsset.h"

namespace Hazel
{
    GPUGraphicsPipelineAsset::~GPUGraphicsPipelineAsset() { GPUGraphicsPipelineAsset::ReleaseImmediate(); }

    void GPUGraphicsPipelineAsset::Release()
    {
        std::unique_lock lock(m_Mutex);
        auto* pipeline = m_Pipeline;
        m_Pipeline = nullptr;
        m_IsLoading = false;
        m_Isvalid = false;
        lock.unlock();
        m_Condition.notify_all();

        if (pipeline) { pipeline->Release(); }
    }

    void GPUGraphicsPipelineAsset::ReleaseImmediate()
    {
        std::unique_lock lock(m_Mutex);
        auto* pipeline = m_Pipeline;
        m_Pipeline = nullptr;
        m_IsLoading = false;
        m_Isvalid = false;
        lock.unlock();
        m_Condition.notify_all();

        if (pipeline) { pipeline->ReleaseImmediate(); }
    }
} // namespace Hazel