//
// Created by helmholtz on 2026/3/29.
//

#include "ComputeShaderAsset.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/ShaderCommon.h"

namespace Hazel
{
    YAML::Node ComputeShaderAssetMeta::Serialize() const
    {
        YAML::Node rootNode;
        rootNode["UUID"] = static_cast<uint64_t>(uuid);
        return rootNode;
    }

    ComputeShaderAssetMeta ComputeShaderAssetMeta::Deserialize(const YAML::Node& node)
    {
        ComputeShaderAssetMeta meta;
        meta.uuid = node["UUID"] ? UUID(node["UUID"].as<uint64_t>()) : UUID();
        return meta;
    }

    ComputeShaderAsset::ComputeShaderAsset(Renderer* renderer,
                                           std::filesystem::path filePath,
                                           ComputeShaderAssetMeta meta)
        : m_Meta(meta),
          m_FilePath(std::move(filePath)),
          m_Renderer(renderer)
    {
        RHIShaderFileDesc shaderFileDesc{};
        shaderFileDesc.path = filePath;
        shaderFileDesc.entryPoint = "main";
        shaderFileDesc.stage = RHIShaderStageFlagBits::Compute;
        shaderFileDesc.debugName = filePath.filename().string() + " [CS]";
        shaderFileDesc.macroDefinitions.push_back({"COMPUTE_SHADER", ""});
        m_CompileResult = CompileShaderFileToSPIRV(shaderFileDesc);
    }

    void ComputeShaderAsset::Load()
    {
        RHIShaderDesc shaderDesc{};
        shaderDesc.stage = RHIShaderStageFlagBits::Compute;
        shaderDesc.entryPoint = "main";
        shaderDesc.debugName = m_FilePath.filename().string() + " [CS]";
        shaderDesc.binary.assign(m_CompileResult.cbegin(), m_CompileResult.cend());
        auto shader = m_Renderer->GetDevice()->CreateShader(shaderDesc);
        m_ComputeShader = m_Renderer->AddComputeShader(std::make_unique<ComputeShader>(m_UUID, shader));
        m_IsLoaded = true;
    }

    void ComputeShaderAsset::Unload()
    {
        if (!m_IsLoaded)
        {
            return;
        }

        m_Renderer->RemoveComputeShader(m_ComputeShader);
        m_ComputeShader = nullptr;
        m_IsLoaded = false;
    }

    ComputeShaderAsset::ComputeShaderAsset(ComputeShaderAsset&& other) noexcept
        : m_IsLoaded(other.m_IsLoaded),
          m_UUID(other.m_UUID),
          m_Meta(other.m_Meta),
          m_FilePath(std::move(other.m_FilePath)),
          m_Renderer(other.m_Renderer),
          m_ComputeShader(other.m_ComputeShader)
    {
        other.m_IsLoaded = false;
        other.m_Renderer = nullptr;
        other.m_ComputeShader = nullptr;
    }

    ComputeShaderAsset& ComputeShaderAsset::operator=(ComputeShaderAsset&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Release();

        m_Meta = other.m_Meta;
        m_FilePath = std::move(other.m_FilePath);
        m_Renderer = other.m_Renderer;
        m_ComputeShader = other.m_ComputeShader;

        other.m_Renderer = nullptr;
        other.m_ComputeShader = nullptr;
        return *this;
    }

    ComputeShaderAsset::~ComputeShaderAsset()
    {
        Release();
    }

    void ComputeShaderAsset::Release()
    {
        Unload();
    }
} // namespace Hazel