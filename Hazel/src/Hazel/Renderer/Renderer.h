#pragma once

// ======== Aster Modify Begin ========
#include "GPUAsset/GPUAssetHandle.h"
#include "GeometryDataRegistry.h"
#include "Hazel/Asset/MaterialAsset.h"
#include "Hazel/Renderer/Camera.h"
#include "Hazel/Renderer/GPUAsset/GPUAssetRegistry.h"
#include "Hazel/Renderer/GPUAsset/GPURenderTextureAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUSamplerAsset.h"
#include "Hazel/Renderer/GPUAsset/GPUTextureAsset.h"
#include "Hazel/Renderer/GraphicsContext.h"
#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/ResourceHeapAllocator.h"
#include "Hazel/Scene/Scene.h"
#include "RenderPipeline.h"
#include "RenderScene.h"
#include "ResourceBindingRegistry.h"

#include <memory>
#include <vector>

namespace Aster
{
    struct RenderBufferDesc;
}

namespace Hazel
{
    class MaterialShaderRegistry;

    class Renderer
    {
      public:
        constexpr static auto SwapchainFormatString = "r.Swapchain.Format";
        constexpr static auto MaxFramesInFlightString = "r.Swapchain.MaxFramesInFlight";
        constexpr static auto GPUAssetGarbageCollectFrameThresholdString = "r.GPUAsset.GarbageCollectFrameThreshold";

        constexpr static int kDefaultGPUAssetGarbageCollectFrameThreshold = 600;

        struct FrameData
        {
            Aster::RHICommandPool* commandPool = nullptr;
            Aster::RHICommandBuffer* commandBuffer = nullptr;

            // updated at runtime no need create at first
            uint32_t frameNumber = 0;
            Aster::RHISyncPoint renderCompleteSyncPoint;
            Aster::RHISyncPoint imageAvailableSyncPoint;
        };

        Renderer(GraphicsContext* graphicsContext, Window* window);

        ~Renderer() { Release(); }

        void BeginSwapchainTargetRendering();
        void EndSwapchainTargetRendering();

        void Render();

        static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

        void OnResize();
        void OnViewportResize(uint32_t width, uint32_t height);

        void BeginFrame();
        void EndFrame();

        GraphicsContext* GetGraphicsContext() const { return m_GraphicsContext; }

        Aster::RHIAdapter GetAdapter() const { return m_GraphicsContext->GetAdapter(); }

        Aster::RHIDevice* GetDevice() const { return m_Device; }

        Aster::RHIInstance* GetInstance() const { return m_Instance; }

        Aster::RHISwapchain* GetSwapchain() const { return m_Swapchain; }

        FrameData& GetFrameData(uint64_t frameIndex) { return m_Frames[frameIndex % m_MaxFramesInFlight]; }

        uint64_t GetCurrentFrameIndex() const { return m_CurrentFrame; }

        uint32_t GetCurrentFrameInFlightIndex() const
        { return static_cast<uint32_t>(m_CurrentFrame % m_MaxFramesInFlight); }

        FrameData& GetCurrentFrameData() { return m_Frames[GetCurrentFrameInFlightIndex()]; }

        uint32_t GetMaxFramesInFlight() const { return m_MaxFramesInFlight; }

        Aster::GPUAsset* GetDefaultGPUAsset(Aster::AssetType type);

        Aster::GPURenderTextureAsset* GetDefaultRenderTexture() const { return m_DefaultRenderTexture.get(); }

        Aster::GPUTextureAsset* GetErrorTexture() const { return m_ErrorTexture.get(); }

        uint32_t GetErrorTextureBindingSlot() const { return m_ErrorTextureBindingSlot; }

        Aster::GPUTextureAsset* GetWhiteTexture() const { return m_WhiteTexture.get(); }

        uint32_t GetWhiteTextureBindingSlot() const { return m_WhiteTextureBindingSlot; }

        Aster::GPUSamplerAsset* GetDefaultSampler() const { return m_DefaultSampler.get(); }

        uint32_t GetDefaultSamplerBindingSlot() const { return m_DefaultSamplerBindingSlot; }

        uint32_t GetWhiteTextureWithDefaultSamplerBindingSlot() const
        { return m_WhiteTextureWithDefaultSamplerBindingSlot; }

        uint32_t RegisterBindlessTexture(Aster::GPUAssetHandle texture);
        uint32_t RegisterBindlessSampler(Aster::GPUAssetHandle sampler);
        uint32_t RegisterBindlessSamplerWithImage(Aster::GPUAssetHandle sampler, Aster::GPUAssetHandle image);
        void UnregisterBindlessTexture(uint32_t slot);
        void UnregisterBindlessSampler(uint32_t slot);
        void UnregisterBindlessSamplerWithImage(uint32_t slot);

        Aster::GPUAssetHandle ResolveGPUAsset(UUID uuid, Aster::AssetType type);
        Aster::GPUAssetHandle ResolveGPUAssetBlocked(UUID uuid, Aster::AssetType type);

        Aster::GPUAssetHandle
        ResolveGPUGraphicsPipeline(UUID material,
                                   const std::vector<Aster::RHIFormat>& colorAttachmentFormats,
                                   const std::vector<Aster::RHIColorBlendAttachmentDesc>& colorBlendAttachments,
                                   Aster::RHIFormat depthStencilFormat);
        Aster::GPUAssetHandle
        ResolveGPUGraphicsPipelineBlocked(UUID material,
                                          const std::vector<Aster::RHIFormat>& colorAttachmentFormats,
                                          const std::vector<Aster::RHIColorBlendAttachmentDesc>& colorBlendAttachments,
                                          Aster::RHIFormat depthStencilFormat);
        Aster::GPUAssetHandle ResolveGPURenderTexture(const Aster::RenderTextureDesc& desc,
                                                      uint64_t lastReferencedFrame = -1);
        Aster::GPUAssetHandle ResolveGPURenderBuffer(const Aster::RenderBufferDesc& desc,
                                                     uint64_t lastReferencedFrame = -1);
        Aster::GPUAssetHandle ResolveGPUSampler(const Aster::RHISamplerDesc& desc, uint64_t lastReferencedFrame = -1);

        uint32_t RegisterMaterial(UUID shader, uint64_t shaderSourceVersion, UUID material);
        void UnregisterMaterial(UUID shader, uint64_t shaderSourceVersion, uint32_t materialID);
        void RegisterShader(UUID uuid, uint64_t sourceVersion, const Aster::RHIShaderReflection& reflection);
        void UnregisterShader(UUID uuid, uint64_t sourceVersion);

        Aster::GPUAssetRegistry* GetGPUAssetRegistry() const { return m_GPUAssetRegistry.get(); }

        Aster::ResourceHeapAllocator* GetResourceHeapAllocator() const { return m_ResourceHeapAllocator.get(); }

        Aster::ResourceBindingRegistry* GetResourceBindingRegistry() const { return m_ResourceBindingRegistry.get(); }

        Aster::GeometryDataRegistry* GetGeometryDataRegistry() const { return m_GeometryDataRegistry.get(); }

        Aster::RenderScene* GetRenderScene() const { return m_RenderScene.get(); }

        void ClearRenderScene() const { m_RenderScene->Clear(); }

        void RunGraphicsPass(Aster::RHICommandBuffer* cmd,
                             const SceneCameraView& camera,
                             const std::vector<Aster::RHIRenderingAttachmentDesc>& colorAttachmentDescriptions,
                             const std::vector<Aster::RHIColorBlendAttachmentDesc>& colorBlendAttachments,
                             const Aster::RHIRenderingAttachmentDesc& depthStencilAttachmentDescription,
                             Aster::RHIRect2D renderArea,
                             Aster::RHIRect2D viewportArea,
                             Aster::RHIRect2D scissorArea);

        void RunGraphicsPass(Aster::RHICommandBuffer* cmd,
                             UUID overrideMaterial,
                             const SceneCameraView& camera,
                             const std::vector<Aster::RHIRenderingAttachmentDesc>& colorAttachmentDescriptions,
                             const std::vector<Aster::RHIColorBlendAttachmentDesc>& colorBlendAttachments,
                             const Aster::RHIRenderingAttachmentDesc& depthStencilAttachmentDescription,
                             Aster::RHIRect2D renderArea,
                             Aster::RHIRect2D viewportArea,
                             Aster::RHIRect2D scissorArea);

        void SetCameras(const std::vector<SceneCameraView>& cameras) { m_Cameras = cameras; }

        void Step() { m_CurrentFrame++; }

        void Release();

      private:
        void CreatePerFrameData();
        void DestroyPerFrameData();
        void CreateSwapchainResources();
        void DestroySwapchainResources();
        void RecreateDefaultRenderTexture();
        void CreateDefaultResources();
        Aster::GPUAssetHandle ResolveGPUAssetWhileLoading(Aster::Asset* asset, Aster::AssetType type);
        template <typename TAsset, typename... Args> Aster::GPUAssetHandle ResolveDirectGPUAsset(Args&&... args);
        template <typename TAsset, typename... Args> Aster::GPUAssetHandle ResolveDirectGPUAssetBlocked(Args&&... args);
        std::unique_ptr<Aster::GPUAsset> LoadGPUAsset(Aster::Asset* asset);

        // synchronized from global setting
        uint32_t m_MaxFramesInFlight = 3;
        Aster::RHIFormat m_SwapchainImageFormat = Aster::RHIFormat::BGRA8UNorm;
        uint64_t m_GPUAssetGarbageCollectFrameThreshold = kDefaultGPUAssetGarbageCollectFrameThreshold;

        // backup pointers
        GraphicsContext* m_GraphicsContext = nullptr;
        Aster::RHIInstance* m_Instance = nullptr;
        Aster::RHIDevice* m_Device = nullptr;

        // swapchain, final render target
        uint64_t m_CurrentFrame = 0;
        Window* m_Window = nullptr;
        Aster::RHISurface* m_WindowSurface = nullptr;
        Aster::RHISwapchain* m_Swapchain = nullptr;
        uint32_t m_ViewportWidth = 1920, m_ViewportHeight = 1080;
        std::vector<FrameData> m_Frames;
        UUID m_DefaultRenderTextureUUID = UUID();
        std::unique_ptr<Aster::GPURenderTextureAsset> m_DefaultRenderTexture;

        // GPU asset and rendering registries
        std::unique_ptr<Aster::GPUAssetRegistry> m_GPUAssetRegistry = nullptr;
        std::unique_ptr<Aster::ResourceHeapAllocator> m_ResourceHeapAllocator = nullptr;
        std::unique_ptr<Aster::ResourceBindingRegistry> m_ResourceBindingRegistry = nullptr;
        std::unique_ptr<Aster::GeometryDataRegistry> m_GeometryDataRegistry = nullptr;
        std::unique_ptr<Aster::RenderScene> m_RenderScene = nullptr;
        std::unique_ptr<Aster::RenderPipeline> m_RenderPipeline = nullptr;

        // default resources
        UUID m_ErrorTextureUUID = UUID();
        uint32_t m_ErrorTextureBindingSlot = 0;
        std::unique_ptr<Aster::GPUTextureAsset> m_ErrorTexture;
        UUID m_WhiteTextureUUID = UUID();
        uint32_t m_WhiteTextureBindingSlot = 0;
        std::unique_ptr<Aster::GPUTextureAsset> m_WhiteTexture;
        UUID m_DefaultSamplerUUID = UUID();
        uint32_t m_DefaultSamplerBindingSlot = 0;
        std::unique_ptr<Aster::GPUSamplerAsset> m_DefaultSampler;
        uint32_t m_WhiteTextureWithDefaultSamplerBindingSlot = 0;

        std::vector<SceneCameraView> m_Cameras;
    };
} // namespace Hazel

#include "RendererDirectGPUAsset.inl"
// ======== Aster Modify End ========
