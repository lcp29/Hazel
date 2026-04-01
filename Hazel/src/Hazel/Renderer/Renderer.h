#pragma once

#include "Hazel/Renderer/ComputeShader.h"
#include "Hazel/Renderer/Camera.h"
#include "Hazel/Renderer/GraphicsContext.h"
#include "Hazel/Renderer/RenderTexture.h"
#include "Hazel/Renderer/Sampler.h"
#include "Hazel/Renderer/Shader.h"
#include "Hazel/Renderer/Texture.h"
#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/RenderBuffer.h"
#include "Hazel/Renderer/Mesh.h"

#include <memory>

namespace Hazel
{
    class Renderer
    {
    public:
        constexpr static auto SwapchainFormatString = "r.Swapchain.Format";
        constexpr static auto MaxFramesInFlightString = "r.Swapchain.MaxFramesInFlight";

        struct FrameData
        {
            RHICommandPool* commandPool = nullptr;
            RHICommandBuffer* commandBuffer = nullptr;

            // updated runtime no need create at first
            uint32_t frameNumber = 0;
            RHISyncPoint renderCompleteSyncPoint;
            RHISyncPoint imageAvailableSyncPoint;
        };

        Renderer(GraphicsContext* graphicsContext, Window* window);

        void BeginSwapchainTargetRendering();
        void EndSwapchainTargetRendering();
        // void Render(RenderScene* renderScene, Camera& camera);
        void Render(Camera& camera);

        static RendererAPI::API GetAPI()
        {
            return RendererAPI::GetAPI();
        }

        void OnResize();
        void OnViewportResize(uint32_t width, uint32_t height);

        void BeginFrame();
        void EndFrame();

        GraphicsContext* GetGraphicsContext() const
        {
            return m_GraphicsContext;
        }

        RHIAdapter GetAdapter() const
        {
            return m_GraphicsContext->GetAdapter();
        }

        RHIDevice* GetDevice() const
        {
            return m_Device;
        }

        RHIInstance* GetInstance() const
        {
            return m_Instance;
        }

        RHISwapchain* GetSwapchain() const
        {
            return m_Swapchain;
        }

        FrameData& GetFrameData(uint64_t frameIndex)
        {
            return m_Frames[frameIndex % m_MaxFramesInFlight];
        }

        uint64_t GetCurrentFrameIndex() const
        {
            return m_CurrentFrame;
        }

        uint64_t GetCurrentFrameInFlightIndex() const
        {
            return m_CurrentFrame % m_MaxFramesInFlight;
        }

        FrameData& GetCurrentFrameData()
        {
            return m_Frames[GetCurrentFrameInFlightIndex()];
        }

        int GetMaxFramesInFlight() const
        {
            return m_MaxFramesInFlight;
        }

        RenderTexture* GetDefaultRenderTexture() const
        {
            return m_DefaultRenderTexture.get();
        }

        Mesh* AddMesh(std::unique_ptr<Mesh> mesh);
        void RemoveMesh(Mesh* mesh);

        RenderTexture* AddRenderTexture(std::unique_ptr<RenderTexture> renderTexture);
        void RemoveRenderTexture(RenderTexture* renderTexture);

        Sampler* AddSampler(std::unique_ptr<Sampler> sampler);
        void RemoveSampler(Sampler* sampler);

        Texture* AddTexture(std::unique_ptr<Texture> texture);
        void RemoveTexture(Texture* texture);

        Shader* AddShader(std::unique_ptr<Shader> shader);
        void RemoveShader(Shader* shader);

        ComputeShader* AddComputeShader(std::unique_ptr<ComputeShader> computeShader);
        void RemoveComputeShader(ComputeShader* computeShader);

        RenderBuffer* AddRenderBuffer(std::unique_ptr<RenderBuffer> renderBuffer);
        void RemoveRenderBuffer(RenderBuffer* renderBuffer);

        Texture* GetErrorTexture()
        {
            return m_ErrorTexture.get();
        }

        Sampler* GetDefaultSampler()
        {
            return m_DefaultSampler.get();
        }

        void Step()
        {
            m_CurrentFrame++;
        }

    private:
        void CreatePerFrameData();
        void DestroyPerFrameData();
        void CreateSwapchainResources();
        void DestroySwapchainResources();
        void RecreateDefaultRenderTexture();
        void CreateDefaultResources();

        // synchronized from global setting
        uint32_t m_MaxFramesInFlight = 3;
        RHIFormat m_SwapchainImageFormat = RHIFormat::BGRA8UNorm;

        // backup pointers
        GraphicsContext* m_GraphicsContext = nullptr;
        RHIInstance* m_Instance = nullptr;
        RHIDevice* m_Device = nullptr;

        // swapchain, final render target and views
        uint64_t m_CurrentFrame = 0;
        Window* m_Window = nullptr;
        RHISurface* m_WindowSurface = nullptr;
        RHISwapchain* m_Swapchain = nullptr;
        uint32_t m_ViewportWidth = 1280, m_ViewportHeight = 720;
        std::vector<FrameData> m_Frames;
        UUID m_DefaultRenderTextureUUID = UUID();
        std::unique_ptr<RenderTexture> m_DefaultRenderTexture;
        RHIOwnerSet<RenderTexture> m_OffscreenRenderTextures;
        RHIOwnerSet<Sampler> m_Samplers;
        RHIOwnerSet<Texture> m_Textures;
        RHIOwnerSet<Shader> m_Shaders;
        RHIOwnerSet<ComputeShader> m_ComputeShaders;
        RHIOwnerSet<Mesh> m_Meshes;
        RHIOwnerSet<RenderBuffer> m_RenderBuffers;

        // default resources
        UUID m_ErrorTextureUUID = UUID();
        std::unique_ptr<Texture> m_ErrorTexture;
        UUID m_WhiteTextureUUID = UUID();
        std::unique_ptr<Texture> m_WhiteTexture;
        UUID m_DefaultSamplerUUID = UUID();
        std::unique_ptr<Sampler> m_DefaultSampler;
    };
} // namespace Hazel
