#pragma once

#include "Hazel/Scene/Scene.h"
#include "GeometryDataRegistry.h"
#include "ResourceBindingRegistry.h"
#include "RenderScene.h"
#include "GPUAsset/GPUAssetResolveResult.h"
#include "Hazel/Asset/MaterialAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUAssetRegistry.h"
#include "Hazel/Renderer/Camera.h"
#include "Hazel/Renderer/GraphicsContext.h"
#include "Hazel/Renderer/GPUAsset/GPURenderTextureAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUSamplerAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUTextureAsset.h"
#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/ResourceHeapAllocator.h"

#include <memory>
#include <vector>

namespace Hazel
{
    struct RenderBufferDesc;
    class MaterialAsset;
    class MaterialShaderRegistry;

    class Renderer
    {
    public:
        constexpr static auto SwapchainFormatString = "r.Swapchain.Format";
        constexpr static auto MaxFramesInFlightString = "r.Swapchain.MaxFramesInFlight";
        constexpr static auto GPUAssetGarbageCollectFrameThresholdString =
            "r.GPUAsset.GarbageCollectFrameThreshold";

        constexpr static int kDefaultGPUAssetGarbageCollectFrameThreshold = 600;

        struct FrameData
        {
            RHICommandPool* commandPool = nullptr;
            RHICommandBuffer* commandBuffer = nullptr;

            // updated at runtime no need create at first
            uint32_t frameNumber = 0;
            RHISyncPoint renderCompleteSyncPoint;
            RHISyncPoint imageAvailableSyncPoint;
        };

        Renderer(GraphicsContext* graphicsContext, Window* window);
        ~Renderer();

        void BeginSwapchainTargetRendering();
        void EndSwapchainTargetRendering();

        void Render();

        static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

        void OnResize();
        void OnViewportResize(uint32_t width, uint32_t height);

        void BeginFrame();
        void EndFrame();

        GraphicsContext* GetGraphicsContext() const { return m_GraphicsContext; }
        RHIAdapter GetAdapter() const { return m_GraphicsContext->GetAdapter(); }
        RHIDevice* GetDevice() const { return m_Device; }
        RHIInstance* GetInstance() const { return m_Instance; }
        RHISwapchain* GetSwapchain() const { return m_Swapchain; }

        FrameData& GetFrameData(uint64_t frameIndex) { return m_Frames[frameIndex % m_MaxFramesInFlight]; }
        uint64_t GetCurrentFrameIndex() const { return m_CurrentFrame; }
        uint64_t GetCurrentFrameInFlightIndex() const { return m_CurrentFrame % m_MaxFramesInFlight; }
        FrameData& GetCurrentFrameData() { return m_Frames[GetCurrentFrameInFlightIndex()]; }
        int GetMaxFramesInFlight() const { return m_MaxFramesInFlight; }

        GPUAsset* GetDefaultGPUAsset(AssetType type);
        GPURenderTextureAsset* GetDefaultRenderTexture() const { return m_DefaultRenderTexture.get(); }

        GPUTextureAsset* GetErrorTexture() const { return m_ErrorTexture.get(); }
        uint32_t GetErrorTextureBindingSlot() const { return m_ErrorTextureBindingSlot; }
        GPUTextureAsset* GetWhiteTexture() const { return m_WhiteTexture.get(); }
        uint32_t GetWhiteTextureBindingSlot() const { return m_WhiteTextureBindingSlot; }
        GPUSamplerAsset* GetDefaultSampler() const { return m_DefaultSampler.get(); }
        uint32_t GetDefaultSamplerBindingSlot() const { return m_DefaultSamplerBindingSlot; }

        uint32_t GetWhiteTextureWithDefaultSamplerBindingSlot() const
        {
            return m_WhiteTextureWithDefaultSamplerBindingSlot;
        }

        uint32_t RegisterBindlessTexture(GPUAssetResolveResult texture);
        uint32_t RegisterBindlessSampler(GPUAssetResolveResult sampler);
        uint32_t RegisterBindlessSamplerWithImage(GPUAssetResolveResult sampler, GPUAssetResolveResult image);
        void UnregisterBindlessTexture(uint32_t slot);
        void UnregisterBindlessSampler(uint32_t slot);
        void UnregisterBindlessSamplerWithImage(uint32_t slot);

        GPUAssetResolveResult ResolveGPUAsset(UUID uuid, AssetType type);
        GPUAssetResolveResult ResolveGPUAssetBlocked(UUID uuid, AssetType type);

        GPUAssetResolveResult ResolveGPUGraphicsPipeline(UUID material,
                                                         const std::vector<RHIFormat>& colorAttachmentFormats,
                                                         const std::vector<RHIColorBlendAttachmentDesc>&
                                                         colorBlendAttachments,
                                                         RHIFormat depthStencilFormat);
        GPUAssetResolveResult ResolveGPUGraphicsPipelineBlocked(UUID material,
                                                                const std::vector<RHIFormat>& colorAttachmentFormats,
                                                                const std::vector<RHIColorBlendAttachmentDesc>&
                                                                colorBlendAttachments,
                                                                RHIFormat depthStencilFormat);
        GPUAssetResolveResult ResolveGPURenderTexture(const RenderTextureDesc& desc,
                                                      uint64_t lastReferencedFrame = -1);
        GPUAssetResolveResult ResolveGPURenderBuffer(const RenderBufferDesc& desc,
                                                     uint64_t lastReferencedFrame = -1);
        GPUAssetResolveResult ResolveGPUSampler(const RHISamplerDesc& desc,
                                                uint64_t lastReferencedFrame = -1);

        uint32_t RegisterMaterial(UUID shader, uint64_t shaderSourceVersion, UUID material);
        void UnregisterMaterial(UUID shader, uint64_t shaderSourceVersion, uint32_t materialID);
        void RegisterShader(UUID uuid, uint64_t sourceVersion, const RHIShaderReflection& reflection);
        void UnregisterShader(UUID uuid, uint64_t sourceVersion);

        ResourceHeapAllocator* GetResourceHeapAllocator() const { return m_ResourceHeapAllocator.get(); }
        ResourceBindingRegistry* GetResourceBindingRegistry() const { return m_ResourceBindingManager.get(); }
        GeometryDataRegistry* GetGeometryDataRegistry() const { return m_GeometryDataRegistry.get(); }

        RenderScene* GetRenderScene() const { return m_RenderScene.get(); }
        void ClearRenderScene() const { m_RenderScene->Clear(); }

        void RunGraphicsPass(RHICommandBuffer* cmd,
                             SceneCameraView* camera,
                             const std::vector<RHIRenderingAttachmentDesc>& colorAttachmentDescriptions,
                             const std::vector<RHIColorBlendAttachmentDesc>& colorBlendAttachments,
                             const RHIRenderingAttachmentDesc* depthStencilAttachmentDescription,
                             RHIOffset2D renderOffset,
                             RHIExtent2D renderViewSize);

        void RunGraphicsPass(RHICommandBuffer* cmd,
                             UUID overrideMaterial,
                             SceneCameraView* camera,
                             const std::vector<RHIRenderingAttachmentDesc>& colorAttachmentDescriptions,
                             const std::vector<RHIColorBlendAttachmentDesc>& colorBlendAttachments,
                             const RHIRenderingAttachmentDesc* depthStencilAttachmentDescription,
                             RHIOffset2D renderOffset,
                             RHIExtent2D renderViewSize);

        // TODO: TEMP URGENT INTERVIEW
        void SetCameras(const std::vector<SceneCameraView> cameras) { m_Cameras = cameras; }

        void Step() { m_CurrentFrame++; }

        void Release();

    private:
        void CreatePerFrameData();
        void DestroyPerFrameData();
        void CreateSwapchainResources();
        void DestroySwapchainResources();
        void RecreateDefaultRenderTexture();
        void CreateDefaultResources();
        GPUAssetResolveResult ResolveGPUAssetWhileLoading(Asset* asset, AssetType type);
        template <typename TAsset, typename... Args>
        GPUAssetResolveResult ResolveDirectGPUAsset(Args&&... args);
        template <typename TAsset, typename... Args>
        GPUAssetResolveResult ResolveDirectGPUAssetBlocked(Args&&... args);
        std::unique_ptr<GPUAsset> LoadGPUAsset(Asset* asset);

        // synchronized from global setting
        uint32_t m_MaxFramesInFlight = 3;
        RHIFormat m_SwapchainImageFormat = RHIFormat::BGRA8UNorm;
        uint64_t m_GPUAssetGarbageCollectFrameThreshold = kDefaultGPUAssetGarbageCollectFrameThreshold;

        // backup pointers
        GraphicsContext* m_GraphicsContext = nullptr;
        RHIInstance* m_Instance = nullptr;
        RHIDevice* m_Device = nullptr;

        // swapchain, final render target
        uint64_t m_CurrentFrame = 0;
        Window* m_Window = nullptr;
        RHISurface* m_WindowSurface = nullptr;
        RHISwapchain* m_Swapchain = nullptr;
        uint32_t m_ViewportWidth = 1280, m_ViewportHeight = 720;
        std::vector<FrameData> m_Frames;
        UUID m_DefaultRenderTextureUUID = UUID();
        std::unique_ptr<GPURenderTextureAsset> m_DefaultRenderTexture;
        std::unique_ptr<GPURenderTextureAsset> m_DefaultDepthRenderTexture;

        // TODO: refactored gpu asset management below
        GPUAssetRegistry m_GPUAssetRegistry;
        std::unique_ptr<ResourceHeapAllocator> m_ResourceHeapAllocator = nullptr;
        std::unique_ptr<ResourceBindingRegistry> m_ResourceBindingManager = nullptr;
        std::unique_ptr<GeometryDataRegistry> m_GeometryDataRegistry = nullptr;
        std::unique_ptr<RenderScene> m_RenderScene = nullptr;

        // default resources
        UUID m_ErrorTextureUUID = UUID();
        uint32_t m_ErrorTextureBindingSlot = 0;
        std::unique_ptr<GPUTextureAsset> m_ErrorTexture;
        UUID m_WhiteTextureUUID = UUID();
        uint32_t m_WhiteTextureBindingSlot = 0;
        std::unique_ptr<GPUTextureAsset> m_WhiteTexture;
        UUID m_DefaultSamplerUUID = UUID();
        uint32_t m_DefaultSamplerBindingSlot = 0;
        std::unique_ptr<GPUSamplerAsset> m_DefaultSampler;
        uint32_t m_WhiteTextureWithDefaultSamplerBindingSlot = 0;

        // TODO: TEMP URGENT INTERVIEW
        std::vector<SceneCameraView> m_Cameras;
    };
} // namespace Hazel

#include "RendererDirectGPUAsset.inl"
