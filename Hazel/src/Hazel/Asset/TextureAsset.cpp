//
// Created by helmholtz on 2026/3/24.
//

#include "TextureAsset.h"

#include "Hazel/Renderer/Renderer.h"

namespace Hazel
{
    YAML::Node TextureAssetMeta::Serialize() const
    {
        YAML::Node rootNode;
        rootNode["UUID"] = static_cast<uint64_t>(uuid);
        rootNode["IsSRGB"] = isSRGB;
        rootNode["UseMipmap"] = useMipmap;
        return rootNode;
    }

    TextureAssetMeta TextureAssetMeta::Deserialize(const YAML::Node& node)
    {
        TextureAssetMeta meta;

        meta.uuid = node["UUID"] ? UUID(node["UUID"].as<uint64_t>()) : UUID();
        meta.isSRGB = node["IsSRGB"] ? node["IsSRGB"].as<bool>() : true;
        meta.useMipmap = node["UseMipmap"] ? node["UseMipmap"].as<bool>() : false;

        return meta;
    }

    void TextureAsset::Load()
    {
        if (m_IsLoaded)
        {
            return;
        }

        RHIImageUsages imageUsages = RHIImageUsageFlagBits::Sampled;
        if (m_Meta.useMipmap)
        {
            imageUsages |= RHIImageUsageFlagBits::TransferDestination;
            imageUsages |= RHIImageUsageFlagBits::TransferSource;
        }

        auto cmd = m_Renderer->GetGraphicsContext()->GetDefaultCommandBuffer();
        auto queue = m_Renderer->GetDevice()->GetUniformQueue();

        cmd->Reset();
        cmd->Begin(true);

        auto image = RHIImage::Factory::CreateFromFile(
            m_Renderer->GetDevice(),
            cmd,
            m_FilePath,
            m_Meta.isSRGB,
            m_Meta.useMipmap,
            imageUsages);

        cmd->End();
        RHIQueueSubmitDesc submitDesc{};
        submitDesc.commandBuffers = {cmd};
        auto syncPoint = queue->Submit(submitDesc);
        m_Renderer->GetDevice()->WaitSyncPoint(&syncPoint);

        cmd->Reset();
        cmd->Begin(true);

        RHIImageViewDesc imageViewDesc{};
        imageViewDesc.format = image->GetDesc().format;
        imageViewDesc.subresourceRange.baseMipLevel = 0;
        imageViewDesc.subresourceRange.levelCount = image->GetDesc().mipLevels;

        auto imageView = m_Renderer->GetDevice()->CreateImageView(image, imageViewDesc);

        TextureDesc textureDesc{};
        textureDesc.format = image->GetDesc().format;
        textureDesc.width = image->GetDesc().width;
        textureDesc.height = image->GetDesc().height;
        textureDesc.useMipmap = m_Meta.useMipmap;
        textureDesc.useMipmap = imageUsages;

        auto texture = std::make_unique<Texture>(m_UUID, textureDesc, m_Renderer, image, imageView);
        m_Texture = m_Renderer->AddTexture(std::move(texture));
        if (m_Meta.useMipmap)
        {
            m_Texture->GenerateMipmap(cmd);
        }

        cmd->End();
        submitDesc.commandBuffers = {cmd};
        syncPoint = queue->Submit(submitDesc);
        m_Renderer->GetDevice()->WaitSyncPoint(&syncPoint);

        m_IsLoaded = true;
    }

    void TextureAsset::Unload()
    {
        if (!m_IsLoaded)
        {
            return;
        }
        m_Renderer->RemoveTexture(m_Texture);
        m_Texture = nullptr;
        m_IsLoaded = false;
    }

    TextureAsset::TextureAsset(TextureAsset&& other) noexcept
        : m_UUID(other.m_UUID)
          , m_IsLoaded(other.m_IsLoaded)
          , m_FilePath(std::move(other.m_FilePath))
          , m_Renderer(other.m_Renderer)
          , m_Meta(other.m_Meta)
          , m_Texture(other.m_Texture)
    {
        other.m_Renderer = nullptr;
        other.m_Texture = nullptr;
        other.m_IsLoaded = false;
    }

    TextureAsset& TextureAsset::operator=(TextureAsset&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Release();

        m_UUID = other.m_UUID;
        m_IsLoaded = other.m_IsLoaded;
        m_FilePath = std::move(other.m_FilePath);
        m_Renderer = other.m_Renderer;
        m_Meta = other.m_Meta;
        m_Texture = other.m_Texture;

        other.m_IsLoaded = false;
        other.m_Renderer = nullptr;
        other.m_Texture = nullptr;
        return *this;
    }

    TextureAsset::~TextureAsset()
    {
        Release();
    }

    void TextureAsset::Release()
    {
        Unload();
    }

    void TextureAsset::Recreate()
    {
        if (m_IsLoaded)
        {
            m_Renderer->GetDevice()->WaitIdle();
            Unload();
            Load();
        }
    }
} // namespace Hazel