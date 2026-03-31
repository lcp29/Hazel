//
// Created by helmholtz on 2026/3/31.
//

#include "ShaderAsset.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/ShaderCommon.h"

namespace Hazel
{
    YAML::Node ShaderAssetMeta::Serialize() const
    {
        YAML::Node rootNode;
        rootNode["UUID"] = static_cast<uint64_t>(uuid);
        return rootNode;
    }

    ShaderAssetMeta ShaderAssetMeta::Deserialize(const YAML::Node& node)
    {
        ShaderAssetMeta meta;
        meta.uuid = node["UUID"] ? UUID(node["UUID"].as<uint64_t>()) : UUID();
        return meta;
    }

    ShaderAsset::ShaderAsset(Renderer* renderer,
                             std::filesystem::path filePath,
                             ShaderAssetMeta meta)
        : m_UUID(meta.uuid),
          m_Meta(meta),
          m_FilePath(std::move(filePath)),
          m_Renderer(renderer)
    {
        RHIShaderFileDesc vertexShaderFileDesc{};
        vertexShaderFileDesc.path = filePath;
        vertexShaderFileDesc.entryPoint = "main";
        vertexShaderFileDesc.stage = RHIShaderStageFlagBits::Vertex;
        vertexShaderFileDesc.debugName = filePath.filename().string() + " [VS]";
        vertexShaderFileDesc.macroDefinitions.push_back({"VERTEX_SHADER", ""});
        m_VertexCompileResult = CompileShaderFileToSPIRV(vertexShaderFileDesc);

        RHIShaderFileDesc fragmentShaderFileDesc{};
        fragmentShaderFileDesc.path = filePath;
        fragmentShaderFileDesc.entryPoint = "main";
        fragmentShaderFileDesc.stage = RHIShaderStageFlagBits::Fragment;
        fragmentShaderFileDesc.debugName = filePath.filename().string() + " [FS]";
        fragmentShaderFileDesc.macroDefinitions.push_back({"FRAGMENT_SHADER", ""});
        m_FragmentCompileResult = CompileShaderFileToSPIRV(fragmentShaderFileDesc);
    }

    void ShaderAsset::Load()
    {
        RHIShaderDesc vertexShaderDesc{};
        vertexShaderDesc.stage = RHIShaderStageFlagBits::Vertex;
        vertexShaderDesc.entryPoint = "main";
        vertexShaderDesc.debugName = m_FilePath.filename().string() + " [VS]";
        vertexShaderDesc.binary.assign(m_VertexCompileResult.cbegin(), m_VertexCompileResult.cend());
        auto vertexShader = m_Renderer->GetDevice()->CreateShader(vertexShaderDesc);

        RHIShaderDesc fragmentShaderDesc{};
        fragmentShaderDesc.stage = RHIShaderStageFlagBits::Fragment;
        fragmentShaderDesc.entryPoint = "main";
        fragmentShaderDesc.debugName = m_FilePath.filename().string() + " [FS]";
        fragmentShaderDesc.binary.assign(m_FragmentCompileResult.cbegin(), m_FragmentCompileResult.cend());
        auto fragmentShader = m_Renderer->GetDevice()->CreateShader(fragmentShaderDesc);

        m_Shader = m_Renderer->AddShader(std::make_unique<Shader>(m_UUID, vertexShader, fragmentShader));
        m_IsLoaded = true;
    }

    void ShaderAsset::Unload()
    {
        if (!m_IsLoaded)
        {
            return;
        }

        m_Renderer->RemoveShader(m_Shader);
        m_Shader = nullptr;
        m_IsLoaded = false;
    }

    ShaderAsset::ShaderAsset(ShaderAsset&& other) noexcept
        : m_IsLoaded(other.m_IsLoaded),
          m_UUID(other.m_UUID),
          m_Meta(other.m_Meta),
          m_FilePath(std::move(other.m_FilePath)),
          m_VertexCompileResult(std::move(other.m_VertexCompileResult)),
          m_FragmentCompileResult(std::move(other.m_FragmentCompileResult)),
          m_Renderer(other.m_Renderer),
          m_Shader(other.m_Shader)
    {
        other.m_IsLoaded = false;
        other.m_Renderer = nullptr;
        other.m_Shader = nullptr;
    }

    ShaderAsset& ShaderAsset::operator=(ShaderAsset&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Release();

        m_IsLoaded = other.m_IsLoaded;
        m_UUID = other.m_UUID;
        m_Meta = other.m_Meta;
        m_FilePath = std::move(other.m_FilePath);
        m_VertexCompileResult = std::move(other.m_VertexCompileResult);
        m_FragmentCompileResult = std::move(other.m_FragmentCompileResult);
        m_Renderer = other.m_Renderer;
        m_Shader = other.m_Shader;

        other.m_IsLoaded = false;
        other.m_Renderer = nullptr;
        other.m_Shader = nullptr;
        return *this;
    }

    ShaderAsset::~ShaderAsset()
    {
        Release();
    }

    void ShaderAsset::Release()
    {
        Unload();
    }
} // namespace Hazel
