//
// Created by helmholtz on 2026/3/15.
//

#include <exception>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "Hazel/RHI/RHI.h"
#include "Hazel/RHI/RHIFactory.h"

namespace Hazel
{
    namespace
    {
        void DebugMessageCallback(const DebugMessage &msg, void *)
        {
            std::cout << "[Vulkan] " << msg.message << '\n';
        }

        bool Expect(bool condition, const std::string &message)
        {
            if (!condition)
            {
                std::cerr << "[FAIL] " << message << '\n';
                return false;
            }

            std::cout << "[PASS] " << message << '\n';
            return true;
        }

        template<typename T>
        bool ReleaseAndForget(T *&object, const std::string &message)
        {
            if (!object)
            {
                return Expect(false, message);
            }

            object->Release();
            object = nullptr;
            return Expect(object == nullptr, message);
        }

        template<typename T>
        bool ReleaseImmediateAndForget(T *&object, const std::string &message)
        {
            if (!object)
            {
                return Expect(false, message);
            }

            object->ReleaseImmediate();
            object = nullptr;
            return Expect(object == nullptr, message);
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

        RHIDevice *CreateTestDevice(RHIInstance &instance)
        {
            RHIDeviceCapabilities caps;
            caps.queueTypes = RHIQueueTypeFlagBits::Graphics;

            const auto adapters = instance.GetAdapters();
            auto adapter = SelectAdapter(adapters, caps);
            if (!adapter)
            {
                return nullptr;
            }

            if (adapter->GetCapabilities().queueTypes & RHIQueueTypeFlagBits::Compute)
            {
                caps.queueTypes = caps.queueTypes | RHIQueueTypeFlagBits::Compute;
            }

            return instance.CreateDevice(adapter, caps);
        }

        bool TestCommandPoolAndBuffer(RHIDevice &device)
        {
            RHICommandPoolDesc poolDesc;
            poolDesc.queueType = RHIQueueType::Graphics;
            poolDesc.transient = true;
            poolDesc.allowCommandBufferReset = true;

            auto *commandPool = device.CreateCommandPool(poolDesc);
            if (!Expect(commandPool != nullptr, "Create graphics command pool"))
            {
                return false;
            }
            if (!Expect(commandPool->IsValid(), "Created command pool is valid"))
            {
                return false;
            }

            RHICommandBufferDesc commandBufferDesc;
            commandBufferDesc.level = RHICommandBufferLevel::Primary;

            auto *commandBuffer = commandPool->CreateCommandBuffer(commandBufferDesc);
            if (!Expect(commandBuffer != nullptr, "Allocate primary command buffer"))
            {
                return false;
            }
            if (!Expect(commandBuffer->IsValid(), "Allocated command buffer is valid"))
            {
                return false;
            }
            if (!Expect(commandBuffer->Begin(true), "Begin command buffer recording"))
            {
                return false;
            }
            if (!Expect(commandBuffer->IsRecording(), "Command buffer enters recording state"))
            {
                return false;
            }
            if (!Expect(commandBuffer->End(), "End command buffer recording"))
            {
                return false;
            }
            if (!Expect(!commandBuffer->IsRecording(), "Command buffer exits recording state"))
            {
                return false;
            }
            if (!Expect(commandBuffer->Reset(), "Reset command buffer"))
            {
                return false;
            }

            RHICommandBufferDesc secondaryDesc;
            secondaryDesc.level = RHICommandBufferLevel::Secondary;
            auto *secondaryCommandBuffer = commandPool->CreateCommandBuffer(secondaryDesc);
            if (!Expect(secondaryCommandBuffer != nullptr, "Allocate secondary command buffer"))
            {
                return false;
            }
            if (!Expect(secondaryCommandBuffer->IsValid(), "Allocated secondary command buffer is valid"))
            {
                return false;
            }

            if (!ReleaseAndForget(commandBuffer, "Deferred command buffer release invalidates local pointer"))
            {
                return false;
            }

            if (!ReleaseImmediateAndForget(commandPool, "Immediate command pool release invalidates local pointer"))
            {
                return false;
            }

            secondaryCommandBuffer = nullptr;
            return Expect(secondaryCommandBuffer == nullptr,
                          "Released child command buffer pointer is discarded by the test");
        }

        bool TestManagedResourceBinding(RHIDevice &device)
        {
            RHIResourceLayoutDesc layoutDesc;
            layoutDesc.bindings.push_back({
                0,
                RHIResourceBindingType::Sampler,
                1,
                RHIShaderStageFlagBits::Fragment
            });
            layoutDesc.bindings.push_back({
                1,
                RHIResourceBindingType::SamplerWithImage,
                1,
                RHIShaderStageFlagBits::Fragment
            });
            layoutDesc.bindings.push_back({
                2,
                RHIResourceBindingType::UniformBuffer,
                1,
                RHIShaderStageFlagBits::Vertex
            });
            layoutDesc.bindings.push_back({
                3,
                RHIResourceBindingType::SampledImage,
                1,
                RHIShaderStageFlagBits::Fragment
            });
            auto *layout = device.CreateResourceLayout(layoutDesc);
            if (!Expect(layout != nullptr, "Create resource layout"))
            {
                return false;
            }
            if (!Expect(layout->IsValid(), "Created resource layout is valid"))
            {
                return false;
            }

            RHIResourceSignatureDesc signatureDesc;
            signatureDesc.resourceLayouts = {layout};
            signatureDesc.pushConstantRanges.push_back(RHIPushConstantRangeDesc{
                0,
                16,
                RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment
            });
            auto *signature = device.CreateResourceSignature(signatureDesc);
            if (!Expect(signature != nullptr, "Create resource signature"))
            {
                return false;
            }
            if (!Expect(signature->IsValid(), "Created resource signature is valid"))
            {
                return false;
            }

            RHIResourceHeapDesc heapDesc;
            heapDesc.maxGroups = 2;
            heapDesc.samplerCount = 2;
            heapDesc.samplerWithImageCount = 2;
            heapDesc.uniformBufferCount = 2;
            heapDesc.sampledImageCount = 2;

            auto *heap = device.CreateResourceHeap(heapDesc);
            if (!Expect(heap != nullptr, "Create resource heap"))
            {
                return false;
            }
            if (!Expect(heap->IsValid(), "Created resource heap is valid"))
            {
                return false;
            }

            auto *bindingSet = heap->CreateGroup(layout);
            if (!Expect(bindingSet != nullptr, "Allocate resource binding set"))
            {
                return false;
            }
            if (!Expect(bindingSet->IsValid(), "Allocated resource binding set is valid"))
            {
                return false;
            }

            RHIBufferDesc uniformBufferDesc;
            uniformBufferDesc.size = 64;
            uniformBufferDesc.usages = RHIBufferUsageFlagBits::UniformBuffer;
            uniformBufferDesc.cpuAccess = RHIBufferCpuAccess::Write;
            uniformBufferDesc.mapOnCreate = true;

            auto *uniformBuffer = device.CreateBuffer(uniformBufferDesc);
            if (!Expect(uniformBuffer != nullptr, "Create descriptor-backed uniform buffer"))
            {
                return false;
            }

            auto *mappedUniformData = static_cast<uint8_t *>(uniformBuffer->Map());
            if (!Expect(mappedUniformData != nullptr, "Map descriptor-backed uniform buffer"))
            {
                return false;
            }
            for (uint32_t i = 0; i < 16; i++)
            {
                mappedUniformData[i] = static_cast<uint8_t>(i + 1);
            }

            RHIImageDesc imageDesc;
            imageDesc.width = 32;
            imageDesc.height = 32;
            imageDesc.format = RHIFormat::RGBA8UNorm;
            imageDesc.usages = RHIImageUsageFlagBits::Sampled;
            imageDesc.initialState = RHIImageResourceState::Undefined;

            auto *image = device.CreateImage(imageDesc);
            if (!Expect(image != nullptr, "Create descriptor-backed sampled image"))
            {
                return false;
            }

            RHIImageViewDesc viewDesc;
            viewDesc.format = imageDesc.format;
            viewDesc.viewType = RHIImageViewType::Image2D;
            viewDesc.subresourceRange.planes = RHIImagePlaneFlagBits::Color;
            viewDesc.subresourceRange.levelCount = 1;
            viewDesc.subresourceRange.layerCount = 1;

            auto *imageView = image->CreateView(viewDesc);
            if (!Expect(imageView != nullptr, "Create descriptor-backed image view"))
            {
                return false;
            }

            RHISamplerDesc samplerDesc;
            auto *sampler = device.CreateSampler(samplerDesc);
            if (!Expect(sampler != nullptr, "Create descriptor-backed sampler"))
            {
                return false;
            }
            if (!Expect(sampler->IsValid(), "Created sampler is valid"))
            {
                return false;
            }

            if (!Expect(bindingSet->WriteSampler(0, sampler),
                        "Write sampler descriptor"))
            {
                return false;
            }

            if (!Expect(
                bindingSet->WriteSamplerWithImage(
                    1, sampler, imageView, RHIImageResourceState::ShaderRead),
                "Write combined image sampler descriptor"))
            {
                return false;
            }

            if (!Expect(bindingSet->WriteBuffer(2, uniformBuffer),
                        "Write uniform buffer descriptor"))
            {
                return false;
            }

            if (!Expect(
                bindingSet->WriteImageView(3, imageView, RHIImageResourceState::ShaderRead),
                "Write sampled image descriptor"))
            {
                return false;
            }

            RHICommandPoolDesc commandPoolDesc;
            commandPoolDesc.queueType = RHIQueueType::Graphics;
            commandPoolDesc.allowCommandBufferReset = true;

            auto *commandPool = device.CreateCommandPool(commandPoolDesc);
            if (!Expect(commandPool != nullptr, "Create command pool for push constants"))
            {
                return false;
            }

            auto *commandBuffer = commandPool->CreateCommandBuffer({});
            if (!Expect(commandBuffer != nullptr, "Allocate command buffer for push constants"))
            {
                return false;
            }

            if (!Expect(commandBuffer->Begin(true), "Begin command buffer for push constants"))
            {
                return false;
            }

            const uint32_t pushConstantData[4] = {1, 2, 3, 4};
            if (!Expect(commandBuffer->PushConstants(
                            signature,
                            RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment,
                            0,
                            sizeof(pushConstantData),
                            pushConstantData),
                        "Push constants through resource signature"))
            {
                return false;
            }

            if (!Expect(commandBuffer->End(), "End command buffer after push constants"))
            {
                return false;
            }

            if (!ReleaseAndForget(bindingSet, "Deferred binding set release invalidates local pointer"))
            {
                return false;
            }

            if (!ReleaseImmediateAndForget(heap, "Immediate resource heap release invalidates local pointer"))
            {
                return false;
            }

            if (!ReleaseImmediateAndForget(commandPool, "Immediate command pool release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(sampler, "Immediate sampler release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(image, "Immediate image release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(uniformBuffer, "Immediate uniform buffer release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(signature, "Immediate resource signature release invalidates local pointer"))
            {
                return false;
            }
            return ReleaseImmediateAndForget(layout, "Immediate resource layout release invalidates local pointer");
        }

        bool TestBufferObject(RHIDevice &device)
        {
            RHIBufferDesc uploadBufferDesc;
            uploadBufferDesc.size = 256;
            uploadBufferDesc.usages = RHIBufferUsageFlagBits::TransferSource | RHIBufferUsageFlagBits::VertexBuffer;
            uploadBufferDesc.cpuAccess = RHIBufferCpuAccess::Write;
            uploadBufferDesc.mapOnCreate = true;
            uploadBufferDesc.allowGpuAddress = true;

            auto *uploadBuffer = device.CreateBuffer(uploadBufferDesc);
            if (!Expect(uploadBuffer != nullptr, "Create CPU-visible upload buffer"))
            {
                return false;
            }
            if (!Expect(uploadBuffer->IsValid(), "Created upload buffer is valid"))
            {
                return false;
            }

            auto *mappedData = static_cast<uint8_t *>(uploadBuffer->Map());
            if (!Expect(mappedData != nullptr, "Map upload buffer"))
            {
                return false;
            }

            for (uint32_t i = 0; i < 16; i++)
            {
                mappedData[i] = static_cast<uint8_t>(i);
            }

            if (!Expect(uploadBuffer->IsMapped(), "Upload buffer reports mapped state"))
            {
                return false;
            }

            if (!Expect(uploadBuffer->GetDeviceAddress() != 0, "Upload buffer exposes GPU address"))
            {
                return false;
            }

            uploadBuffer->Unmap();
            if (!Expect(uploadBuffer->IsMapped(), "Persistent upload buffer remains mapped after Unmap call"))
            {
                return false;
            }

            RHIBufferDesc readbackBufferDesc;
            readbackBufferDesc.size = 128;
            readbackBufferDesc.usages = RHIBufferUsageFlagBits::TransferDestination;
            readbackBufferDesc.cpuAccess = RHIBufferCpuAccess::Read;

            auto *readbackBuffer = device.CreateBuffer(readbackBufferDesc);
            if (!Expect(readbackBuffer != nullptr, "Create CPU-readable buffer"))
            {
                return false;
            }
            if (!Expect(readbackBuffer->IsValid(), "Created readback buffer is valid"))
            {
                return false;
            }

            auto *readbackData = readbackBuffer->Map();
            if (!Expect(readbackData != nullptr, "Map readback buffer"))
            {
                return false;
            }

            readbackBuffer->Unmap();
            if (!Expect(!readbackBuffer->IsMapped(), "Non-persistent readback buffer unmaps cleanly"))
            {
                return false;
            }

            if (!ReleaseImmediateAndForget(readbackBuffer, "Immediate readback buffer release invalidates local pointer"))
            {
                return false;
            }

            return ReleaseAndForget(uploadBuffer, "Deferred upload buffer release invalidates local pointer");
        }

        bool TestBasicImageAndView(RHIDevice &device)
        {
            RHIImageDesc imageDesc;
            imageDesc.width = 128;
            imageDesc.height = 128;
            imageDesc.mipLevels = 4;
            imageDesc.arrayLayers = 1;
            imageDesc.format = RHIFormat::RGBA8UNorm;
            imageDesc.usages = RHIImageUsageFlagBits::Sampled | RHIImageUsageFlagBits::TransferDestination;
            imageDesc.initialState = RHIImageResourceState::Undefined;

            auto *image = device.CreateImage(imageDesc);
            if (!Expect(image != nullptr, "Create 2D sampled image"))
            {
                return false;
            }
            if (!Expect(image->IsValid(), "Created image is valid"))
            {
                return false;
            }

            RHIImageViewDesc viewDesc;
            viewDesc.format = imageDesc.format;
            viewDesc.viewType = RHIImageViewType::Image2D;
            viewDesc.subresourceRange.planes = RHIImagePlaneFlagBits::Color;
            viewDesc.subresourceRange.baseMipLevel = 0;
            viewDesc.subresourceRange.levelCount = imageDesc.mipLevels;
            viewDesc.subresourceRange.baseArrayLayer = 0;
            viewDesc.subresourceRange.layerCount = imageDesc.arrayLayers;
            viewDesc.componentMapping.r = RHIImageViewComponent::R;
            viewDesc.componentMapping.g = RHIImageViewComponent::G;
            viewDesc.componentMapping.b = RHIImageViewComponent::B;
            viewDesc.componentMapping.a = RHIImageViewComponent::A;

            auto *view = image->CreateView(viewDesc);
            if (!Expect(view != nullptr, "Create full-range 2D image view"))
            {
                return false;
            }
            if (!Expect(view->IsValid(), "Created 2D image view is valid"))
            {
                return false;
            }

            auto remappedDesc = viewDesc;
            remappedDesc.subresourceRange.baseMipLevel = 1;
            remappedDesc.subresourceRange.levelCount = 1;
            remappedDesc.componentMapping.r = RHIImageViewComponent::B;
            remappedDesc.componentMapping.g = RHIImageViewComponent::G;
            remappedDesc.componentMapping.b = RHIImageViewComponent::R;
            remappedDesc.componentMapping.a = RHIImageViewComponent::One;

            auto *remappedView = image->CreateView(remappedDesc);
            if (!Expect(remappedView != nullptr, "Create remapped subresource image view"))
            {
                return false;
            }
            if (!Expect(remappedView->IsValid(), "Created remapped image view is valid"))
            {
                return false;
            }

            if (!ReleaseImmediateAndForget(remappedView, "Immediate remapped image view release invalidates local pointer"))
            {
                return false;
            }

            if (!ReleaseAndForget(view, "Deferred image view release invalidates local pointer"))
            {
                return false;
            }

            return ReleaseAndForget(image, "Deferred image release invalidates local pointer");
        }

        bool TestCubeImageAndView(RHIDevice &device)
        {
            RHIImageDesc cubeDesc;
            cubeDesc.width = 64;
            cubeDesc.height = 64;
            cubeDesc.arrayLayers = 6;
            cubeDesc.format = RHIFormat::RGBA8UNorm;
            cubeDesc.usages = RHIImageUsageFlagBits::Sampled;
            cubeDesc.initialState = RHIImageResourceState::ShaderRead;

            auto *cubeImage = device.CreateImage(cubeDesc);
            if (!Expect(cubeImage != nullptr, "Create cube-compatible image"))
            {
                return false;
            }
            if (!Expect(cubeImage->IsValid(), "Created cube image is valid"))
            {
                return false;
            }

            RHIImageViewDesc cubeViewDesc;
            cubeViewDesc.format = cubeDesc.format;
            cubeViewDesc.viewType = RHIImageViewType::Cube;
            cubeViewDesc.subresourceRange.planes = RHIImagePlaneFlagBits::Color;
            cubeViewDesc.subresourceRange.baseMipLevel = 0;
            cubeViewDesc.subresourceRange.levelCount = cubeDesc.mipLevels;
            cubeViewDesc.subresourceRange.baseArrayLayer = 0;
            cubeViewDesc.subresourceRange.layerCount = 6;

            auto *cubeView = cubeImage->CreateView(cubeViewDesc);
            if (!Expect(cubeView != nullptr, "Create cube image view"))
            {
                return false;
            }
            if (!Expect(cubeView->IsValid(), "Created cube image view is valid"))
            {
                return false;
            }

            if (!ReleaseImmediateAndForget(cubeImage, "Immediate cube image release invalidates local pointer"))
            {
                return false;
            }

            cubeView = nullptr;
            return Expect(cubeView == nullptr, "Released child image view pointer is discarded by the test");
        }

        bool WriteTextFile(const std::filesystem::path &path, const std::string &contents)
        {
            std::ofstream output(path, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!output)
            {
                return false;
            }

            output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
            return output.good();
        }

        bool TestShaderObject(RHIDevice &device)
        {
            const auto shaderDirectory = std::filesystem::temp_directory_path() / "HazelRHITestShaders";
            std::filesystem::create_directories(shaderDirectory);

            const auto vertexShaderPath = shaderDirectory / "rhi_test.vert";
            const auto fragmentShaderPath = shaderDirectory / "rhi_test.frag";

            const std::string vertexShaderSource = R"(#version 450 core
layout(set = 0, binding = 0) uniform CameraData
{
    mat4 viewProj;
    vec4 eyePosition;
} uCamera;

layout(push_constant) uniform DrawData
{
    mat4 model;
} uDraw;

layout(location = 0) in vec3 a_Position;

void main()
{
    gl_Position = uCamera.viewProj * uDraw.model * vec4(a_Position, 1.0);
}
)";

            const std::string fragmentShaderSource = R"(#version 450 core
layout(set = 0, binding = 1) uniform sampler2D uAlbedo;
layout(set = 1, binding = 0, rgba8) uniform image2D uStorageTarget;

layout(location = 0) out vec4 o_Color;

void main()
{
    vec4 color = texture(uAlbedo, vec2(0.5, 0.5));
    imageStore(uStorageTarget, ivec2(0, 0), color);
    o_Color = color;
}
)";

            if (!Expect(WriteTextFile(vertexShaderPath, vertexShaderSource), "Write temporary vertex shader source"))
            {
                return false;
            }

            if (!Expect(WriteTextFile(fragmentShaderPath, fragmentShaderSource),
                        "Write temporary fragment shader source"))
            {
                return false;
            }

            auto *vertexShader = CreateShaderFromGLSLFile(device, {
                                                             vertexShaderPath,
                                                             RHIShaderStageFlagBits::Vertex,
                                                             "main",
                                                             "RHITestVertex"
                                                         });
            if (!Expect(vertexShader != nullptr, "Create vertex shader from GLSL file"))
            {
                return false;
            }
            if (!Expect(vertexShader->IsValid(), "Created vertex shader is valid"))
            {
                return false;
            }

            auto *fragmentShader = CreateShaderFromGLSLFile(device, {
                                                               fragmentShaderPath,
                                                               RHIShaderStageFlagBits::Fragment,
                                                               "main",
                                                               "RHITestFragment"
                                                           });
            if (!Expect(fragmentShader != nullptr, "Create fragment shader from GLSL file"))
            {
                return false;
            }
            if (!Expect(fragmentShader->IsValid(), "Created fragment shader is valid"))
            {
                return false;
            }

            const auto &vertexReflection = vertexShader->GetReflection();
            if (!Expect(vertexReflection.resourceGroups.size() == 1, "Vertex shader reflects one resource group"))
            {
                return false;
            }
            if (!Expect(vertexReflection.resourceGroups[0].set == 0, "Vertex shader resource group uses set 0"))
            {
                return false;
            }
            if (!Expect(vertexReflection.resourceGroups[0].bindings.size() == 1,
                        "Vertex shader reflects one descriptor binding"))
            {
                return false;
            }

            const auto &cameraBinding = vertexReflection.resourceGroups[0].bindings[0];
            if (!Expect(cameraBinding.binding == 0, "Vertex shader uniform buffer binding index matches source"))
            {
                return false;
            }
            if (!Expect(cameraBinding.type == RHIResourceBindingType::UniformBuffer,
                        "Vertex shader uniform buffer binding type is reflected"))
            {
                return false;
            }
            if (!Expect(cameraBinding.buffer.size == 80, "Vertex shader uniform buffer size is reflected"))
            {
                return false;
            }
            if (!Expect(cameraBinding.buffer.members.size() == 2, "Vertex shader uniform buffer members are reflected"))
            {
                return false;
            }
            if (!Expect(cameraBinding.buffer.members[0].name == "viewProj",
                        "Vertex shader first uniform member name matches source"))
            {
                return false;
            }
            if (!Expect(cameraBinding.buffer.members[0].offset == 0,
                        "Vertex shader first uniform member offset is reflected"))
            {
                return false;
            }
            if (!Expect(cameraBinding.buffer.members[1].name == "eyePosition",
                        "Vertex shader second uniform member name matches source"))
            {
                return false;
            }
            if (!Expect(cameraBinding.buffer.members[1].offset == 64,
                        "Vertex shader second uniform member offset is reflected"))
            {
                return false;
            }
            if (!Expect(vertexReflection.pushConstants.size() == 1, "Vertex shader reflects one push constant block"))
            {
                return false;
            }
            if (!Expect(vertexReflection.pushConstants[0].size == 64, "Push constant size is reflected"))
            {
                return false;
            }
            if (!Expect(vertexReflection.pushConstants[0].members.size() == 1,
                        "Push constant members are reflected"))
            {
                return false;
            }

            const auto &fragmentReflection = fragmentShader->GetReflection();
            if (!Expect(fragmentReflection.resourceGroups.size() == 2, "Fragment shader reflects two resource groups"))
            {
                return false;
            }
            if (!Expect(
                fragmentReflection.resourceGroups[0].bindings[0].type == RHIResourceBindingType::SamplerWithImage,
                "Fragment shader sampled image binding type is reflected"))
            {
                return false;
            }
            if (!Expect(fragmentReflection.resourceGroups[0].bindings[0].variableName == "uAlbedo",
                        "Fragment shader sampled image variable name is reflected"))
            {
                return false;
            }
            if (!Expect(fragmentReflection.resourceGroups[1].bindings[0].type == RHIResourceBindingType::StorageImage,
                        "Fragment shader storage image binding type is reflected"))
            {
                return false;
            }
            if (!Expect(fragmentReflection.resourceGroups[1].bindings[0].variableName == "uStorageTarget",
                        "Fragment shader storage image variable name is reflected"))
            {
                return false;
            }

            if (!ReleaseImmediateAndForget(fragmentShader, "Immediate fragment shader release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(vertexShader, "Immediate vertex shader release invalidates local pointer"))
            {
                return false;
            }
            std::filesystem::remove(vertexShaderPath);
            std::filesystem::remove(fragmentShaderPath);
            std::filesystem::remove(shaderDirectory);

            return Expect(vertexShader == nullptr && fragmentShader == nullptr,
                          "Immediate shader release invalidates local pointers");
        }

        bool TestPipelineObjects(RHIDevice &device)
        {
            const auto shaderDirectory = std::filesystem::temp_directory_path() / "HazelRHITestPipelines";
            std::filesystem::create_directories(shaderDirectory);

            const auto graphicsVertexPath = shaderDirectory / "pipeline_test.vert";
            const auto graphicsFragmentPath = shaderDirectory / "pipeline_test.frag";
            const auto computePath = shaderDirectory / "pipeline_test.comp";

            const std::string graphicsVertexSource = R"(#version 450 core
layout(set = 0, binding = 0) uniform DrawData
{
    vec4 tint;
} uDraw;

void main()
{
    const vec2 positions[3] = vec2[3](
        vec2(-0.5, -0.5),
        vec2( 0.5, -0.5),
        vec2( 0.0,  0.5)
    );
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
)";

            const std::string graphicsFragmentSource = R"(#version 450 core
layout(set = 0, binding = 0) uniform DrawData
{
    vec4 tint;
} uDraw;

layout(location = 0) out vec4 o_Color;

void main()
{
    o_Color = uDraw.tint;
}
)";

            const std::string computeSource = R"(#version 450 core
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(set = 0, binding = 0) buffer DataBuffer
{
    uint value;
} uData;

void main()
{
    uData.value += 1;
}
)";

            if (!Expect(WriteTextFile(graphicsVertexPath, graphicsVertexSource), "Write graphics vertex shader source"))
            {
                return false;
            }
            if (!Expect(WriteTextFile(graphicsFragmentPath, graphicsFragmentSource),
                        "Write graphics fragment shader source"))
            {
                return false;
            }
            if (!Expect(WriteTextFile(computePath, computeSource), "Write compute shader source"))
            {
                return false;
            }

            auto *graphicsVertexShader = CreateShaderFromGLSLFile(device, {
                                                                     graphicsVertexPath,
                                                                     RHIShaderStageFlagBits::Vertex,
                                                                     "main",
                                                                     "RHITestPipelineVS"
                                                                 });
            auto *graphicsFragmentShader = CreateShaderFromGLSLFile(device, {
                                                                       graphicsFragmentPath,
                                                                       RHIShaderStageFlagBits::Fragment,
                                                                       "main",
                                                                       "RHITestPipelineFS"
                                                                   });
            auto *computeShader = CreateShaderFromGLSLFile(device, {
                                                              computePath,
                                                              RHIShaderStageFlagBits::Compute,
                                                              "main",
                                                              "RHITestPipelineCS"
                                                          });

            if (!Expect(graphicsVertexShader != nullptr, "Create graphics vertex shader for pipeline"))
            {
                return false;
            }
            if (!Expect(graphicsFragmentShader != nullptr, "Create graphics fragment shader for pipeline"))
            {
                return false;
            }
            if (!Expect(computeShader != nullptr, "Create compute shader for pipeline"))
            {
                return false;
            }

            RHIResourceLayoutDesc graphicsLayoutDesc;
            graphicsLayoutDesc.bindings.push_back({
                0,
                RHIResourceBindingType::UniformBuffer,
                1,
                RHIShaderStageFlagBits::Vertex | RHIShaderStageFlagBits::Fragment
            });

            auto *graphicsLayout = device.CreateResourceLayout(graphicsLayoutDesc);
            if (!Expect(graphicsLayout != nullptr, "Create graphics pipeline resource layout"))
            {
                return false;
            }

            RHIResourceHeapDesc graphicsHeapDesc;
            graphicsHeapDesc.maxGroups = 1;
            graphicsHeapDesc.uniformBufferCount = 1;
            auto *graphicsHeap = device.CreateResourceHeap(graphicsHeapDesc);
            if (!Expect(graphicsHeap != nullptr, "Create graphics pipeline resource heap"))
            {
                return false;
            }

            auto *graphicsGroup = graphicsHeap->CreateGroup(graphicsLayout);
            if (!Expect(graphicsGroup != nullptr, "Allocate graphics pipeline resource group"))
            {
                return false;
            }

            RHIBufferDesc graphicsUniformBufferDesc;
            graphicsUniformBufferDesc.size = 16;
            graphicsUniformBufferDesc.usages = RHIBufferUsageFlagBits::UniformBuffer;
            graphicsUniformBufferDesc.cpuAccess = RHIBufferCpuAccess::Write;
            graphicsUniformBufferDesc.mapOnCreate = true;
            auto *graphicsUniformBuffer = device.CreateBuffer(graphicsUniformBufferDesc);
            if (!Expect(graphicsUniformBuffer != nullptr, "Create graphics pipeline uniform buffer"))
            {
                return false;
            }

            auto *graphicsUniformData = static_cast<float *>(graphicsUniformBuffer->Map());
            if (!Expect(graphicsUniformData != nullptr, "Map graphics pipeline uniform buffer"))
            {
                return false;
            }
            graphicsUniformData[0] = 1.0f;
            graphicsUniformData[1] = 0.5f;
            graphicsUniformData[2] = 0.25f;
            graphicsUniformData[3] = 1.0f;

            if (!Expect(graphicsGroup->WriteBuffer(0, graphicsUniformBuffer),
                        "Write graphics pipeline resource group"))
            {
                return false;
            }

            RHIResourceSignatureDesc graphicsSignatureDesc;
            graphicsSignatureDesc.resourceLayouts = {graphicsLayout};
            auto *graphicsSignature = device.CreateResourceSignature(graphicsSignatureDesc);
            if (!Expect(graphicsSignature != nullptr, "Create graphics pipeline resource signature"))
            {
                return false;
            }

            RHIGraphicsPipelineDesc graphicsPipelineDesc;
            graphicsPipelineDesc.resourceSignature = graphicsSignature;
            graphicsPipelineDesc.vertexShader = graphicsVertexShader;
            graphicsPipelineDesc.fragmentShader = graphicsFragmentShader;
            graphicsPipelineDesc.colorAttachmentFormats = {RHIFormat::RGBA8UNorm};
            auto *graphicsPipeline = device.CreateGraphicsPipeline(graphicsPipelineDesc);
            if (!Expect(graphicsPipeline != nullptr, "Create graphics pipeline"))
            {
                return false;
            }
            if (!Expect(graphicsPipeline->IsValid(), "Created graphics pipeline is valid"))
            {
                return false;
            }

            RHIResourceLayoutDesc computeLayoutDesc;
            computeLayoutDesc.bindings.push_back({
                0,
                RHIResourceBindingType::StorageBuffer,
                1,
                RHIShaderStageFlagBits::Compute
            });

            auto *computeLayout = device.CreateResourceLayout(computeLayoutDesc);
            if (!Expect(computeLayout != nullptr, "Create compute pipeline resource layout"))
            {
                return false;
            }

            RHIResourceHeapDesc computeHeapDesc;
            computeHeapDesc.maxGroups = 1;
            computeHeapDesc.storageBufferCount = 1;
            auto *computeHeap = device.CreateResourceHeap(computeHeapDesc);
            if (!Expect(computeHeap != nullptr, "Create compute pipeline resource heap"))
            {
                return false;
            }

            auto *computeGroup = computeHeap->CreateGroup(computeLayout);
            if (!Expect(computeGroup != nullptr, "Allocate compute pipeline resource group"))
            {
                return false;
            }

            RHIBufferDesc computeBufferDesc;
            computeBufferDesc.size = sizeof(uint32_t);
            computeBufferDesc.usages = RHIBufferUsageFlagBits::StorageBuffer;
            computeBufferDesc.cpuAccess = RHIBufferCpuAccess::Write;
            computeBufferDesc.mapOnCreate = true;
            auto *computeBuffer = device.CreateBuffer(computeBufferDesc);
            if (!Expect(computeBuffer != nullptr, "Create compute pipeline storage buffer"))
            {
                return false;
            }

            auto *computeData = static_cast<uint32_t *>(computeBuffer->Map());
            if (!Expect(computeData != nullptr, "Map compute pipeline storage buffer"))
            {
                return false;
            }
            computeData[0] = 41;

            if (!Expect(computeGroup->WriteBuffer(0, computeBuffer),
                        "Write compute pipeline resource group"))
            {
                return false;
            }

            RHIResourceSignatureDesc computeSignatureDesc;
            computeSignatureDesc.resourceLayouts = {computeLayout};
            auto *computeSignature = device.CreateResourceSignature(computeSignatureDesc);
            if (!Expect(computeSignature != nullptr, "Create compute pipeline resource signature"))
            {
                return false;
            }

            RHIComputePipelineDesc computePipelineDesc;
            computePipelineDesc.resourceSignature = computeSignature;
            computePipelineDesc.computeShader = computeShader;
            auto *computePipeline = device.CreateComputePipeline(computePipelineDesc);
            if (!Expect(computePipeline != nullptr, "Create compute pipeline"))
            {
                return false;
            }
            if (!Expect(computePipeline->IsValid(), "Created compute pipeline is valid"))
            {
                return false;
            }

            RHICommandPoolDesc graphicsCommandPoolDesc;
            graphicsCommandPoolDesc.queueType = RHIQueueType::Graphics;
            graphicsCommandPoolDesc.allowCommandBufferReset = true;
            auto *graphicsCommandPool = device.CreateCommandPool(graphicsCommandPoolDesc);
            if (!Expect(graphicsCommandPool != nullptr, "Create graphics command pool for pipeline recording"))
            {
                return false;
            }

            auto *graphicsCommandBuffer = graphicsCommandPool->CreateCommandBuffer({});
            if (!Expect(graphicsCommandBuffer != nullptr, "Allocate graphics command buffer for pipeline recording"))
            {
                return false;
            }

            if (!Expect(graphicsCommandBuffer->Begin(true),
                        "Begin graphics command buffer for pipeline recording"))
            {
                return false;
            }

            if (!Expect(graphicsCommandBuffer->BindGraphicsPipeline(graphicsPipeline),
                        "Bind graphics pipeline"))
            {
                return false;
            }
            if (!Expect(graphicsCommandBuffer->BindGraphicsResourceGroup(
                            graphicsPipeline, 0, graphicsGroup),
                        "Bind graphics resource group"))
            {
                return false;
            }
            if (!Expect(graphicsCommandBuffer->SetViewport(0.0f, 0.0f, 128.0f, 128.0f),
                        "Set graphics pipeline viewport"))
            {
                return false;
            }
            if (!Expect(graphicsCommandBuffer->SetScissor(0, 0, 128, 128),
                        "Set graphics pipeline scissor"))
            {
                return false;
            }
            if (!Expect(graphicsCommandBuffer->SetBlendConstants(1.0f, 1.0f, 1.0f, 1.0f),
                        "Set graphics pipeline blend constants"))
            {
                return false;
            }
            if (!Expect(graphicsCommandBuffer->SetStencilReference(0),
                        "Set graphics pipeline stencil reference"))
            {
                return false;
            }

            if (!Expect(graphicsCommandBuffer->End(), "End graphics command buffer after pipeline binding"))
            {
                return false;
            }

            RHICommandPoolDesc computeCommandPoolDesc;
            computeCommandPoolDesc.queueType = device.GetQueue(RHIQueueType::Compute)
                                                   ? RHIQueueType::Compute
                                                   : RHIQueueType::Graphics;
            computeCommandPoolDesc.allowCommandBufferReset = true;
            auto *computeCommandPool = device.CreateCommandPool(computeCommandPoolDesc);
            if (!Expect(computeCommandPool != nullptr, "Create compute command pool for pipeline recording"))
            {
                return false;
            }

            auto *computeCommandBuffer = computeCommandPool->CreateCommandBuffer({});
            if (!Expect(computeCommandBuffer != nullptr, "Allocate compute command buffer for pipeline recording"))
            {
                return false;
            }

            if (!Expect(computeCommandBuffer->Begin(true),
                        "Begin compute command buffer for pipeline recording"))
            {
                return false;
            }

            if (!Expect(computeCommandBuffer->BindComputePipeline(computePipeline),
                        "Bind compute pipeline"))
            {
                return false;
            }
            if (!Expect(computeCommandBuffer->BindComputeResourceGroup(
                            computePipeline, 0, computeGroup),
                        "Bind compute resource group"))
            {
                return false;
            }
            if (!Expect(computeCommandBuffer->End(), "End compute command buffer after pipeline binding"))
            {
                return false;
            }

            if (!ReleaseImmediateAndForget(computeCommandPool, "Immediate compute command pool release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(graphicsCommandPool, "Immediate graphics command pool release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(computePipeline, "Immediate compute pipeline release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(computeBuffer, "Immediate compute buffer release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(computeHeap, "Immediate compute heap release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(computeSignature, "Immediate compute signature release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(computeLayout, "Immediate compute layout release invalidates local pointer"))
            {
                return false;
            }

            if (graphicsPipeline)
            {
                graphicsPipeline->ReleaseImmediate();
                graphicsPipeline = nullptr;
            }
            if (!ReleaseImmediateAndForget(graphicsUniformBuffer, "Immediate graphics uniform buffer release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(graphicsHeap, "Immediate graphics heap release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(graphicsSignature, "Immediate graphics signature release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(graphicsLayout, "Immediate graphics layout release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(computeShader, "Immediate compute shader release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(graphicsFragmentShader, "Immediate graphics fragment shader release invalidates local pointer"))
            {
                return false;
            }
            if (!ReleaseImmediateAndForget(graphicsVertexShader, "Immediate graphics vertex shader release invalidates local pointer"))
            {
                return false;
            }

            std::filesystem::remove(graphicsVertexPath);
            std::filesystem::remove(graphicsFragmentPath);
            std::filesystem::remove(computePath);
            std::filesystem::remove(shaderDirectory);

            return Expect(computePipeline == nullptr && graphicsPipeline == nullptr,
                          "Immediate pipeline release invalidates local pointers");
        }

        bool RunRHITestImpl()
        {
            RHIInstanceDesc desc;
            desc.backend = RHIBackend::Vulkan;
            desc.appName = "RHITest";
            desc.appVersion = {1, 0, 0};
            desc.engineName = "RenderingPlayground";
            desc.engineVersion = {1, 0, 0};
            desc.useValidation = true;
            desc.debugMessageSeverity = DebugMessageSeverityFlagBits::Warning | DebugMessageSeverityFlagBits::Error;
            desc.debugMessageType = DebugMessageTypeFlagBits::Validation | DebugMessageTypeFlagBits::Performance;
            desc.debugMessageCallback = DebugMessageCallback;

            auto instance = CreateInstance(desc);
            if (!Expect(instance.has_value(), "Create Vulkan RHI instance"))
            {
                return false;
            }
            if (!Expect(instance.value()->IsValid(), "Created instance is valid"))
            {
                return false;
            }

            auto *instancePtr = instance.value().get();
            const auto adapters = instancePtr->GetAdapters();
            if (!Expect(!adapters.empty(), "Enumerate at least one Vulkan adapter"))
            {
                return false;
            }

            auto *device = CreateTestDevice(*instancePtr);
            if (!Expect(device != nullptr, "Create Vulkan device with graphics queue"))
            {
                return false;
            }
            if (!Expect(device->IsValid(), "Created device is valid"))
            {
                return false;
            }
            if (!Expect(static_cast<bool>(device->GetQueue(RHIQueueType::Graphics)),
                        "Graphics queue is available"))
            {
                return false;
            }

            if (!TestCommandPoolAndBuffer(*device))
            {
                return false;
            }

            if (!TestManagedResourceBinding(*device))
            {
                return false;
            }

            if (!TestBufferObject(*device))
            {
                return false;
            }

            if (!TestBasicImageAndView(*device))
            {
                return false;
            }

            if (!TestCubeImageAndView(*device))
            {
                return false;
            }

            if (!TestShaderObject(*device))
            {
                return false;
            }

            if (!TestPipelineObjects(*device))
            {
                return false;
            }

            device->Release();
            device = nullptr;
            return Expect(device == nullptr, "Device release completes and local pointer is discarded");
        }
    } // namespace

    bool RunRHITest()
    {
        return RunRHITestImpl();
    }
} // namespace Hazel

int main()
{
    try
    {
        return Hazel::RunRHITest() ? 0 : 1;
    } catch (const std::exception &e)
    {
        std::cerr << "[EXCEPTION] " << e.what() << '\n';
        return 1;
    } catch (...)
    {
        std::cerr << "[EXCEPTION] Unknown exception\n";
        return 1;
    }
}
