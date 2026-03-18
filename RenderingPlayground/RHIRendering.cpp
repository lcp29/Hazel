//
// Created by helmholtz on 2026/3/18.
//

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <shaderc/shaderc.hpp>

#include "Hazel/RHI/RHI.h"
#include "Hazel/RHI/RHIFactory.h"

void DebugMessageCallback(const Hazel::DebugMessage &msg, void *)
{
    std::cout << "[Vulkan] " << msg.message << '\n';
}

namespace Hazel
{
    namespace
    {
        struct Vertex
        {
            glm::vec3 position;
            glm::vec3 color;
        };

        struct alignas(16) SceneUniforms
        {
            glm::mat4 modelViewProjection{1.0f};
        };

        constexpr uint32_t s_WindowWidth = 1280;
        constexpr uint32_t s_WindowHeight = 720;
        constexpr RHIFormat s_DepthFormat = RHIFormat::D32SFloat;

        const char *s_VertexShaderSource = R"(#version 450 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;

layout(location = 0) out vec3 vColor;

layout(set = 0, binding = 0) uniform SceneUniforms
{
    mat4 uModelViewProjection;
};

void main()
{
    vColor = aColor;
    gl_Position = uModelViewProjection * vec4(aPosition, 1.0);
}
)";

        const char *s_FragmentShaderSource = R"(#version 450 core
layout(location = 0) in vec3 vColor;
layout(location = 0) out vec4 oColor;

void main()
{
    oColor = vec4(vColor, 1.0);
}
)";

        const std::array<Vertex, 8> s_CubeVertices = {{
            {{-1.0f, -1.0f, -1.0f}, {0.95f, 0.29f, 0.25f}},
            {{ 1.0f, -1.0f, -1.0f}, {0.96f, 0.62f, 0.18f}},
            {{ 1.0f,  1.0f, -1.0f}, {0.99f, 0.88f, 0.29f}},
            {{-1.0f,  1.0f, -1.0f}, {0.30f, 0.78f, 0.43f}},
            {{-1.0f, -1.0f,  1.0f}, {0.19f, 0.68f, 0.90f}},
            {{ 1.0f, -1.0f,  1.0f}, {0.32f, 0.45f, 0.93f}},
            {{ 1.0f,  1.0f,  1.0f}, {0.65f, 0.37f, 0.93f}},
            {{-1.0f,  1.0f,  1.0f}, {0.93f, 0.38f, 0.71f}},
        }};

        const std::array<uint16_t, 36> s_CubeIndices = {{
            0, 1, 2, 2, 3, 0,
            4, 6, 5, 6, 4, 7,
            0, 4, 5, 5, 1, 0,
            3, 2, 6, 6, 7, 3,
            1, 5, 6, 6, 2, 1,
            0, 3, 7, 7, 4, 0
        }};

        [[noreturn]] void Fail(const std::string &message)
        {
            throw std::runtime_error(message);
        }

        void Check(bool condition, const std::string &message)
        {
            if (!condition)
            {
                Fail(message);
            }
        }

        const RHIAdapter *SelectAdapter(const std::vector<RHIAdapter> &adapters, const RHIDeviceCapabilities &caps)
        {
            for (const auto &adapter: adapters)
            {
                if (adapter.CanCreateDevice(caps))
                {
                    return &adapter;
                }
            }

            return nullptr;
        }

        std::vector<uint32_t> CompileShaderToSpirv(std::string_view source,
                                                   RHIShaderStageFlagBits stage,
                                                   const char *debugName)
        {
            shaderc_shader_kind shaderKind = shaderc_glsl_infer_from_source;
            switch (stage)
            {
                case RHIShaderStageFlagBits::Vertex:
                    shaderKind = shaderc_glsl_vertex_shader;
                    break;
                case RHIShaderStageFlagBits::Fragment:
                    shaderKind = shaderc_glsl_fragment_shader;
                    break;
                case RHIShaderStageFlagBits::Compute:
                    shaderKind = shaderc_glsl_compute_shader;
                    break;
            }

            shaderc::Compiler compiler;
            shaderc::CompileOptions options;
            options.SetSourceLanguage(shaderc_source_language_glsl);
            options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);

            auto result = compiler.CompileGlslToSpv(source.data(),
                                                    source.size(),
                                                    shaderKind,
                                                    debugName,
                                                    "main",
                                                    options);
            if (result.GetCompilationStatus() != shaderc_compilation_status_success)
            {
                Fail(std::string("Shader compilation failed for ") + debugName + ": " + result.GetErrorMessage());
            }

            return {result.cbegin(), result.cend()};
        }

        RHIShader *CreateEmbeddedShader(RHIDevice &device,
                                        std::string_view source,
                                        RHIShaderStageFlagBits stage,
                                        const char *debugName)
        {
            RHIShaderDesc desc;
            desc.stage = stage;
            desc.entryPoint = "main";
            desc.debugName = debugName;
            desc.binary = CompileShaderToSpirv(source, stage, debugName);
            return device.CreateShader(desc);
        }

        glm::mat4 BuildProjection(float aspectRatio)
        {
            auto projection = glm::perspective(glm::radians(55.0f), aspectRatio, 0.1f, 100.0f);
            projection[1][1] *= -1.0f;
            return projection;
        }

        void UpdateSceneUniforms(RHIBuffer &uniformBuffer, float aspectRatio, float angleRadians)
        {
            auto *mappedData = static_cast<SceneUniforms *>(uniformBuffer.Map());
            Check(mappedData != nullptr, "Failed to map scene uniform buffer");

            const glm::mat4 model = glm::rotate(glm::mat4(1.0f), angleRadians, glm::vec3(0.45f, 1.0f, 0.15f));
            const glm::mat4 view = glm::lookAt(glm::vec3(2.8f, 2.2f, 4.8f),
                                               glm::vec3(0.0f, 0.0f, 0.0f),
                                               glm::vec3(0.0f, 1.0f, 0.0f));
            mappedData->modelViewProjection = BuildProjection(aspectRatio) * view * model;
        }

        void UploadBufferData(RHIBuffer &buffer, const void *data, size_t size)
        {
            auto *mappedData = buffer.Map();
            Check(mappedData != nullptr, "Failed to map buffer for upload");
            std::memcpy(mappedData, data, size);
        }

        std::vector<vk::ImageView> CreateSwapchainImageViews(RHIDevice &device, RHISwapchain &swapchain)
        {
            std::vector<vk::ImageView> imageViews;
            imageViews.reserve(swapchain.GetImages().size());

            for (const auto image: swapchain.GetImages())
            {
                vk::ImageViewCreateInfo createInfo;
                createInfo.image = image;
                createInfo.viewType = vk::ImageViewType::e2D;
                createInfo.format = VulkanConvertFormat(swapchain.GetFormat());
                createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount = 1;

                const auto imageView = device.GetHandle().createImageView(createInfo);
                Check(static_cast<bool>(imageView), "Failed to create swapchain image view");
                imageViews.push_back(imageView);
            }

            return imageViews;
        }

        void DestroySwapchainImageViews(RHIDevice &device, std::vector<vk::ImageView> &imageViews)
        {
            for (const auto imageView: imageViews)
            {
                if (imageView)
                {
                    device.GetHandle().destroyImageView(imageView);
                }
            }
            imageViews.clear();
        }

        vk::AccessFlags2 GetAccessMask(RHIImageResourceState state)
        {
            switch (state)
            {
                case RHIImageResourceState::Undefined:
                    return {};
                case RHIImageResourceState::Common:
                    return vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
                case RHIImageResourceState::TransferSource:
                    return vk::AccessFlagBits2::eTransferRead;
                case RHIImageResourceState::TransferDestination:
                    return vk::AccessFlagBits2::eTransferWrite;
                case RHIImageResourceState::ShaderRead:
                    return vk::AccessFlagBits2::eShaderRead;
                case RHIImageResourceState::ShaderWrite:
                    return vk::AccessFlagBits2::eShaderWrite;
                case RHIImageResourceState::ColorAttachment:
                    return vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite;
                case RHIImageResourceState::DepthStencilAttachment:
                    return vk::AccessFlagBits2::eDepthStencilAttachmentRead
                           | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
                case RHIImageResourceState::Present:
                    return {};
            }

            return {};
        }

        RHIPipelineStages GetPipelineStages(RHIImageResourceState state)
        {
            switch (state)
            {
                case RHIImageResourceState::Undefined:
                    return RHIPipelineStageFlagBits::Top;
                case RHIImageResourceState::Common:
                    return RHIPipelineStageFlagBits::AllCommands;
                case RHIImageResourceState::TransferSource:
                case RHIImageResourceState::TransferDestination:
                    return RHIPipelineStageFlagBits::Transfer;
                case RHIImageResourceState::ShaderRead:
                case RHIImageResourceState::ShaderWrite:
                    return RHIPipelineStageFlagBits::AllCommands;
                case RHIImageResourceState::ColorAttachment:
                    return RHIPipelineStageFlagBits::ColorAttachmentOutput;
                case RHIImageResourceState::DepthStencilAttachment:
                    return RHIPipelineStageFlagBits::EarlyDepthStencil
                           | RHIPipelineStageFlagBits::LateDepthStencil;
                case RHIImageResourceState::Present:
                    return RHIPipelineStageFlagBits::Bottom;
            }

            return RHIPipelineStageFlagBits::AllCommands;
        }

        void TransitionSwapchainImage(vk::CommandBuffer commandBuffer,
                                      vk::Image image,
                                      RHIImageResourceState oldState,
                                      RHIImageResourceState newState,
                                      uint32_t srcQueueFamilyIndex,
                                      uint32_t dstQueueFamilyIndex)
        {
            vk::ImageMemoryBarrier2 barrier;
            barrier.srcStageMask = VulkanConvertPipelineStages(GetPipelineStages(oldState));
            barrier.srcAccessMask = GetAccessMask(oldState);
            barrier.dstStageMask = VulkanConvertPipelineStages(GetPipelineStages(newState));
            barrier.dstAccessMask = GetAccessMask(newState);
            barrier.oldLayout = VulkanConvertResourceState(oldState);
            barrier.newLayout = VulkanConvertResourceState(newState);
            barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
            barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            vk::DependencyInfo dependencyInfo;
            dependencyInfo.imageMemoryBarrierCount = 1;
            dependencyInfo.pImageMemoryBarriers = &barrier;
            commandBuffer.pipelineBarrier2(dependencyInfo);
        }
    } // namespace

    int RunRHIRendering()
    {
        Check(glfwInit() == GLFW_TRUE, "Failed to initialize GLFW");
        Check(glfwVulkanSupported() == GLFW_TRUE, "GLFW Vulkan support is unavailable");

        GLFWwindow *window = nullptr;
        RHISurface *surface = nullptr;
        RHIDevice *device = nullptr;
        RHISwapchain *swapchain = nullptr;
        RHIImage *depthImage = nullptr;
        RHIImageView *depthView = nullptr;
        RHIBuffer *vertexBuffer = nullptr;
        RHIBuffer *indexBuffer = nullptr;
        RHIBuffer *uniformBuffer = nullptr;
        RHIResourceLayout *resourceLayout = nullptr;
        RHIResourceSignature *resourceSignature = nullptr;
        RHIResourceHeap *resourceHeap = nullptr;
        RHIResourceGroup *resourceGroup = nullptr;
        RHIShader *vertexShader = nullptr;
        RHIShader *fragmentShader = nullptr;
        RHIGraphicsPipeline *graphicsPipeline = nullptr;
        RHICommandPool *commandPool = nullptr;
        RHICommandBuffer *commandBuffer = nullptr;
        std::vector<vk::ImageView> swapchainImageViews;

        try
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
            window = glfwCreateWindow(static_cast<int>(s_WindowWidth),
                                      static_cast<int>(s_WindowHeight),
                                      "RHIRendering",
                                      nullptr,
                                      nullptr);
            Check(window != nullptr, "Failed to create GLFW window");

            RHIInstanceDesc instanceDesc;
            instanceDesc.backend = RHIBackend::Vulkan;
            instanceDesc.appName = "RHIRendering";
            instanceDesc.appVersion = {1, 0, 0};
            instanceDesc.engineName = "RenderingPlayground";
            instanceDesc.engineVersion = {1, 0, 0};
            instanceDesc.useValidation = false;
            instanceDesc.debugMessageSeverity = DebugMessageSeverityFlagBits::Warning | DebugMessageSeverityFlagBits::Error;
            instanceDesc.debugMessageType = DebugMessageTypeFlagBits::Validation | DebugMessageTypeFlagBits::Performance;
            instanceDesc.debugMessageCallback = ::DebugMessageCallback;

            bool vulkanSupported = glfwVulkanSupported();
            Check(vulkanSupported, "Vulkan is not supported on this system");

            auto instanceOwner = CreateInstance(instanceDesc);
            Check(instanceOwner.has_value() && instanceOwner.value() && instanceOwner.value()->IsValid(),
                  "Failed to create Vulkan RHI instance");
            auto *instance = instanceOwner.value().get();

            VkSurfaceKHR glfwSurface = VK_NULL_HANDLE;
            const VkResult surfaceResult = glfwCreateWindowSurface(static_cast<VkInstance>(instance->GetHandle()),
                                                                   window,
                                                                   nullptr,
                                                                   &glfwSurface);
            Check(surfaceResult == VK_SUCCESS && glfwSurface != VK_NULL_HANDLE, "Failed to create GLFW Vulkan surface");

            RHISurfaceDesc surfaceDesc;
            surfaceDesc.backendHandle = glfwSurface;
            surface = instance->CreateSurface(surfaceDesc);
            Check(surface != nullptr && surface->IsValid(), "Failed to create RHI surface");

            RHIDeviceCapabilities deviceCaps;
            deviceCaps.queueTypes = RHIQueueTypeFlagBits::Graphics | RHIQueueTypeFlagBits::Present;

            const auto adapters = instance->GetAdapters();
            const auto *adapter = SelectAdapter(adapters, deviceCaps);
            Check(adapter != nullptr, "Failed to find adapter with graphics and present support");

            device = instance->CreateDevice(adapter, deviceCaps, surface);
            Check(device != nullptr && device->IsValid(), "Failed to create Vulkan RHI device");

            auto *graphicsQueue = device->GetQueue(RHIQueueType::Graphics);
            auto *presentQueue = device->GetQueue(RHIQueueType::Present);
            Check(graphicsQueue != nullptr && presentQueue != nullptr, "Failed to get graphics/present queues");

            int framebufferWidth = 0;
            int framebufferHeight = 0;
            glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
            Check(framebufferWidth > 0 && framebufferHeight > 0, "Invalid framebuffer size");

            RHISwapchainDesc swapchainDesc;
            swapchainDesc.surface = surface;
            swapchainDesc.width = static_cast<uint32_t>(framebufferWidth);
            swapchainDesc.height = static_cast<uint32_t>(framebufferHeight);
            swapchainDesc.imageCount = 2;
            swapchainDesc.format = RHIFormat::RGBA8UNorm;
            swapchainDesc.mode = RHISwapchainMode::FIFO;
            swapchainDesc.usages = RHIImageUsageFlagBits::ColorAttachment;

            swapchain = device->CreateSwapchain(swapchainDesc);
            Check(swapchain != nullptr && swapchain->IsValid(), "Failed to create swapchain");
            swapchainImageViews = CreateSwapchainImageViews(*device, *swapchain);

            RHIImageDesc depthImageDesc;
            depthImageDesc.width = swapchainDesc.width;
            depthImageDesc.height = swapchainDesc.height;
            depthImageDesc.format = s_DepthFormat;
            depthImageDesc.usages = RHIImageUsageFlagBits::DepthStencilAttachment;
            depthImageDesc.initialState = RHIImageResourceState::Undefined;
            depthImage = device->CreateImage(depthImageDesc);
            Check(depthImage != nullptr && depthImage->IsValid(), "Failed to create depth image");

            RHIImageViewDesc depthViewDesc;
            depthViewDesc.format = s_DepthFormat;
            depthViewDesc.viewType = RHIImageViewType::Image2D;
            depthViewDesc.subresourceRange.planes = RHIImagePlaneFlagBits::Depth;
            depthViewDesc.subresourceRange.levelCount = 1;
            depthViewDesc.subresourceRange.layerCount = 1;
            depthView = depthImage->CreateView(depthViewDesc);
            Check(depthView != nullptr && depthView->IsValid(), "Failed to create depth view");

            RHIBufferDesc vertexBufferDesc;
            vertexBufferDesc.size = sizeof(s_CubeVertices);
            vertexBufferDesc.usages = RHIBufferUsageFlagBits::VertexBuffer;
            vertexBufferDesc.cpuAccess = RHIBufferCpuAccess::Write;
            vertexBufferDesc.mapOnCreate = true;
            vertexBuffer = device->CreateBuffer(vertexBufferDesc);
            Check(vertexBuffer != nullptr && vertexBuffer->IsValid(), "Failed to create vertex buffer");
            UploadBufferData(*vertexBuffer, s_CubeVertices.data(), sizeof(s_CubeVertices));

            RHIBufferDesc indexBufferDesc;
            indexBufferDesc.size = sizeof(s_CubeIndices);
            indexBufferDesc.usages = RHIBufferUsageFlagBits::IndexBuffer;
            indexBufferDesc.cpuAccess = RHIBufferCpuAccess::Write;
            indexBufferDesc.mapOnCreate = true;
            indexBuffer = device->CreateBuffer(indexBufferDesc);
            Check(indexBuffer != nullptr && indexBuffer->IsValid(), "Failed to create index buffer");
            UploadBufferData(*indexBuffer, s_CubeIndices.data(), sizeof(s_CubeIndices));

            RHIBufferDesc uniformBufferDesc;
            uniformBufferDesc.size = sizeof(SceneUniforms);
            uniformBufferDesc.usages = RHIBufferUsageFlagBits::UniformBuffer;
            uniformBufferDesc.cpuAccess = RHIBufferCpuAccess::Write;
            uniformBufferDesc.mapOnCreate = true;
            uniformBuffer = device->CreateBuffer(uniformBufferDesc);
            Check(uniformBuffer != nullptr && uniformBuffer->IsValid(), "Failed to create uniform buffer");

            RHIResourceLayoutDesc layoutDesc;
            layoutDesc.bindings.push_back({
                0,
                RHIResourceBindingType::UniformBuffer,
                1,
                RHIShaderStageFlagBits::Vertex
            });
            resourceLayout = device->CreateResourceLayout(layoutDesc);
            Check(resourceLayout != nullptr && resourceLayout->IsValid(), "Failed to create resource layout");

            RHIResourceSignatureDesc signatureDesc;
            signatureDesc.resourceLayouts = {resourceLayout};
            resourceSignature = device->CreateResourceSignature(signatureDesc);
            Check(resourceSignature != nullptr && resourceSignature->IsValid(), "Failed to create resource signature");

            RHIResourceHeapDesc heapDesc;
            heapDesc.maxGroups = 1;
            heapDesc.uniformBufferCount = 1;
            resourceHeap = device->CreateResourceHeap(heapDesc);
            Check(resourceHeap != nullptr && resourceHeap->IsValid(), "Failed to create resource heap");

            resourceGroup = resourceHeap->CreateGroup(resourceLayout);
            Check(resourceGroup != nullptr && resourceGroup->IsValid(), "Failed to create resource group");
            Check(resourceGroup->WriteBuffer(0, uniformBuffer), "Failed to bind uniform buffer");

            vertexShader = CreateEmbeddedShader(*device, s_VertexShaderSource, RHIShaderStageFlagBits::Vertex, "RHIRenderingVS");
            fragmentShader = CreateEmbeddedShader(*device, s_FragmentShaderSource, RHIShaderStageFlagBits::Fragment, "RHIRenderingFS");
            Check(vertexShader != nullptr && fragmentShader != nullptr, "Failed to create shaders");

            RHIGraphicsPipelineDesc pipelineDesc;
            pipelineDesc.resourceSignature = resourceSignature;
            pipelineDesc.vertexShader = vertexShader;
            pipelineDesc.fragmentShader = fragmentShader;
            pipelineDesc.vertexBindings = {
                {0, sizeof(Vertex), RHIVertexInputRate::Vertex}
            };
            pipelineDesc.vertexAttributes = {
                {0, 0, RHIFormat::RGB32SFloat, 0},
                {1, 0, RHIFormat::RGB32SFloat, 12}
            };
            pipelineDesc.topology = RHIPrimitiveTopology::TriangleList;
            pipelineDesc.cullMode = RHICullMode::Back;
            pipelineDesc.frontFace = RHIFrontFace::Clockwise;
            pipelineDesc.depthTestEnable = true;
            pipelineDesc.depthWriteEnable = true;
            pipelineDesc.depthCompareOp = RHICompareOp::LessOrEqual;
            pipelineDesc.colorAttachmentFormats = {swapchain->GetFormat()};
            pipelineDesc.depthStencilFormat = s_DepthFormat;
            graphicsPipeline = device->CreateGraphicsPipeline(pipelineDesc);
            Check(graphicsPipeline != nullptr && graphicsPipeline->IsValid(), "Failed to create graphics pipeline");

            RHICommandPoolDesc commandPoolDesc;
            commandPoolDesc.queueType = RHIQueueType::Graphics;
            commandPoolDesc.allowCommandBufferReset = true;
            commandPool = device->CreateCommandPool(commandPoolDesc);
            Check(commandPool != nullptr && commandPool->IsValid(), "Failed to create command pool");

            commandBuffer = commandPool->CreateCommandBuffer({});
            Check(commandBuffer != nullptr && commandBuffer->IsValid(), "Failed to create command buffer");

            const bool separatePresentQueue = graphicsQueue->GetFamilyIndex() != presentQueue->GetFamilyIndex();
            std::vector<bool> swapchainImageInitialized(swapchain->GetImageCount(), false);
            bool depthInitialized = false;

            const auto startTime = std::chrono::steady_clock::now();
            while (!glfwWindowShouldClose(window))
            {
                glfwPollEvents();
                Check(device->WaitIdle(), "Device wait idle failed");

                const auto acquireResult = swapchain->AcquireImage();
                Check(acquireResult.availableSyncPoint.valid, "Failed to acquire swapchain image");

                const uint32_t frameIndex = acquireResult.frameNumber;
                Check(frameIndex < swapchain->GetImageCount(), "Acquired image index is out of bounds");

                const auto now = std::chrono::steady_clock::now();
                const float elapsedSeconds = std::chrono::duration<float>(now - startTime).count();
                UpdateSceneUniforms(*uniformBuffer,
                                    static_cast<float>(swapchainDesc.width) / static_cast<float>(swapchainDesc.height),
                                    elapsedSeconds * glm::radians(55.0f));

                Check(commandBuffer->Reset(), "Failed to reset command buffer");
                Check(commandBuffer->Begin(true), "Failed to begin command buffer");

                if (!depthInitialized)
                {
                    Check(depthImage->Transition(commandBuffer,
                                                 RHIImageResourceState::Undefined,
                                                 RHIImageResourceState::DepthStencilAttachment),
                          "Failed to transition depth image");
                    depthInitialized = true;
                }

                TransitionSwapchainImage(commandBuffer->GetHandle(),
                                         swapchain->GetImages()[frameIndex],
                                         swapchainImageInitialized[frameIndex]
                                             ? RHIImageResourceState::Present
                                             : RHIImageResourceState::Undefined,
                                         RHIImageResourceState::ColorAttachment,
                                         separatePresentQueue ? presentQueue->GetFamilyIndex() : VK_QUEUE_FAMILY_IGNORED,
                                         separatePresentQueue ? graphicsQueue->GetFamilyIndex() : VK_QUEUE_FAMILY_IGNORED);

                vk::RenderingAttachmentInfo colorAttachment;
                colorAttachment.imageView = swapchainImageViews[frameIndex];
                colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
                colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
                colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
                colorAttachment.clearValue.color = vk::ClearColorValue(std::array<float, 4>{0.05f, 0.07f, 0.11f, 1.0f});

                vk::RenderingAttachmentInfo depthAttachment;
                depthAttachment.imageView = depthView->GetHandle();
                depthAttachment.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
                depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
                depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
                depthAttachment.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

                vk::RenderingInfo renderingInfo;
                renderingInfo.renderArea = vk::Rect2D({0, 0}, {swapchainDesc.width, swapchainDesc.height});
                renderingInfo.layerCount = 1;
                renderingInfo.colorAttachmentCount = 1;
                renderingInfo.pColorAttachments = &colorAttachment;
                renderingInfo.pDepthAttachment = &depthAttachment;

                commandBuffer->GetHandle().beginRendering(renderingInfo);
                Check(commandBuffer->BindGraphicsPipeline(graphicsPipeline), "Failed to bind graphics pipeline");
                Check(commandBuffer->BindGraphicsResourceGroup(graphicsPipeline, 0, resourceGroup),
                      "Failed to bind graphics resource group");
                Check(commandBuffer->BindVertexBuffer(0, vertexBuffer), "Failed to bind vertex buffer");
                Check(commandBuffer->BindIndexBuffer(indexBuffer, RHIIndexType::UInt16), "Failed to bind index buffer");
                Check(commandBuffer->SetViewport(0.0f,
                                                 0.0f,
                                                 static_cast<float>(swapchainDesc.width),
                                                 static_cast<float>(swapchainDesc.height)),
                      "Failed to set viewport");
                Check(commandBuffer->SetScissor(0, 0, swapchainDesc.width, swapchainDesc.height),
                      "Failed to set scissor");
                commandBuffer->GetHandle().drawIndexed(static_cast<uint32_t>(s_CubeIndices.size()), 1, 0, 0, 0);
                commandBuffer->GetHandle().endRendering();

                TransitionSwapchainImage(commandBuffer->GetHandle(),
                                         swapchain->GetImages()[frameIndex],
                                         RHIImageResourceState::ColorAttachment,
                                         RHIImageResourceState::Present,
                                         separatePresentQueue ? graphicsQueue->GetFamilyIndex() : VK_QUEUE_FAMILY_IGNORED,
                                         separatePresentQueue ? presentQueue->GetFamilyIndex() : VK_QUEUE_FAMILY_IGNORED);

                Check(commandBuffer->End(), "Failed to end command buffer");

                RHIQueueSubmitDesc submitDesc;
                submitDesc.commandBuffers = {commandBuffer};
                submitDesc.waitSyncPoints = {acquireResult.availableSyncPoint};
                const auto renderSyncPoint = graphicsQueue->Submit(submitDesc);
                Check(renderSyncPoint.valid, "Failed to submit graphics work");

                Check(swapchain->SubmitFrame(frameIndex, {renderSyncPoint}), "Failed to present frame");
                swapchainImageInitialized[frameIndex] = true;
            }

            Check(device->WaitIdle(), "Device wait idle during shutdown failed");
            DestroySwapchainImageViews(*device, swapchainImageViews);

            if (commandPool)
            {
                commandPool->ReleaseImmediate();
                commandPool = nullptr;
                commandBuffer = nullptr;
            }
            if (graphicsPipeline)
            {
                graphicsPipeline->ReleaseImmediate();
                graphicsPipeline = nullptr;
            }
            if (fragmentShader)
            {
                fragmentShader->ReleaseImmediate();
                fragmentShader = nullptr;
            }
            if (vertexShader)
            {
                vertexShader->ReleaseImmediate();
                vertexShader = nullptr;
            }
            if (resourceHeap)
            {
                resourceHeap->ReleaseImmediate();
                resourceHeap = nullptr;
                resourceGroup = nullptr;
            }
            if (resourceSignature)
            {
                resourceSignature->ReleaseImmediate();
                resourceSignature = nullptr;
            }
            if (resourceLayout)
            {
                resourceLayout->ReleaseImmediate();
                resourceLayout = nullptr;
            }
            if (uniformBuffer)
            {
                uniformBuffer->ReleaseImmediate();
                uniformBuffer = nullptr;
            }
            if (indexBuffer)
            {
                indexBuffer->ReleaseImmediate();
                indexBuffer = nullptr;
            }
            if (vertexBuffer)
            {
                vertexBuffer->ReleaseImmediate();
                vertexBuffer = nullptr;
            }
            if (depthImage)
            {
                depthImage->ReleaseImmediate();
                depthImage = nullptr;
                depthView = nullptr;
            }
            if (swapchain)
            {
                swapchain->Release();
                swapchain = nullptr;
            }
            if (device)
            {
                device->Release();
                device = nullptr;
            }
            if (surface)
            {
                surface->Release();
                surface = nullptr;
            }
            if (window)
            {
                glfwDestroyWindow(window);
                window = nullptr;
            }
            glfwTerminate();
            return 0;
        }
        catch (...)
        {
            if (device)
            {
                (void) device->WaitIdle();
                if (!swapchainImageViews.empty())
                {
                    DestroySwapchainImageViews(*device, swapchainImageViews);
                }
            }

            if (commandPool)
            {
                commandPool->ReleaseImmediate();
                commandPool = nullptr;
                commandBuffer = nullptr;
            }
            if (graphicsPipeline)
            {
                graphicsPipeline->ReleaseImmediate();
                graphicsPipeline = nullptr;
            }
            if (fragmentShader)
            {
                fragmentShader->ReleaseImmediate();
                fragmentShader = nullptr;
            }
            if (vertexShader)
            {
                vertexShader->ReleaseImmediate();
                vertexShader = nullptr;
            }
            if (resourceHeap)
            {
                resourceHeap->ReleaseImmediate();
                resourceHeap = nullptr;
                resourceGroup = nullptr;
            }
            if (resourceSignature)
            {
                resourceSignature->ReleaseImmediate();
                resourceSignature = nullptr;
            }
            if (resourceLayout)
            {
                resourceLayout->ReleaseImmediate();
                resourceLayout = nullptr;
            }
            if (uniformBuffer)
            {
                uniformBuffer->ReleaseImmediate();
                uniformBuffer = nullptr;
            }
            if (indexBuffer)
            {
                indexBuffer->ReleaseImmediate();
                indexBuffer = nullptr;
            }
            if (vertexBuffer)
            {
                vertexBuffer->ReleaseImmediate();
                vertexBuffer = nullptr;
            }
            if (depthImage)
            {
                depthImage->ReleaseImmediate();
                depthImage = nullptr;
                depthView = nullptr;
            }
            if (swapchain)
            {
                swapchain->Release();
                swapchain = nullptr;
            }
            if (device)
            {
                device->Release();
                device = nullptr;
            }
            if (surface)
            {
                surface->Release();
                surface = nullptr;
            }
            if (window)
            {
                glfwDestroyWindow(window);
            }
            glfwTerminate();
            throw;
        }
    }
} // namespace Hazel

int main()
{
    try
    {
        return Hazel::RunRHIRendering();
    }
    catch (const std::exception &e)
    {
        std::cerr << "[EXCEPTION] " << e.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "[EXCEPTION] Unknown exception\n";
        return 1;
    }
}
