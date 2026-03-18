//
// Created by helmholtz on 2026/3/16.
//

#define VULKAN_HPP_NO_EXCEPTIONS

#include "VulkanGraphicsPipeline.h"

#include "VulkanDevice.h"
#include "VulkanPipelineCommon.h"
#include "VulkanResourceSignature.h"
#include "VulkanShader.h"

namespace Hazel
{
    RHI_VK_FUNC_IMPL(RHIGraphicsPipeline, RHIGraphicsPipelineImpl)(RHIDevice *deviceOwner,
                                                                   vk::Device device,
                                                                   const RHIGraphicsPipelineDesc &desc)
    {
        m_DeviceOwner = deviceOwner;
        m_Device = device;
        m_Desc = desc;

        if (!m_DeviceOwner || !m_Device || !desc.vertexShader || !desc.fragmentShader || desc.colorAttachmentFormats.empty())
        {
            return;
        }

        auto *vertexShader = desc.vertexShader;
        auto *fragmentShader = desc.fragmentShader;
        if (!vertexShader || !fragmentShader || !vertexShader->IsValid() || !fragmentShader->IsValid())
        {
            return;
        }

        auto *resourceSignature = desc.resourceSignature;
        if (!resourceSignature || !resourceSignature->IsValid())
        {
            return;
        }
        m_ResourceSignature = resourceSignature;

        const vk::PipelineShaderStageCreateInfo shaderStages[] = {
            {
                {},
                vk::ShaderStageFlagBits::eVertex,
                vertexShader->GetHandle(),
                desc.vertexShader->GetEntryPoint().c_str()
            },
            {
                {},
                vk::ShaderStageFlagBits::eFragment,
                fragmentShader->GetHandle(),
                desc.fragmentShader->GetEntryPoint().c_str()
            }
        };

        std::vector<vk::VertexInputBindingDescription> vertexBindings;
        vertexBindings.reserve(desc.vertexBindings.size());
        for (const auto &binding: desc.vertexBindings)
        {
            vertexBindings.emplace_back(binding.binding, binding.stride, VulkanConvertVertexInputRate(binding.inputRate));
        }

        std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
        vertexAttributes.reserve(desc.vertexAttributes.size());
        for (const auto &attribute: desc.vertexAttributes)
        {
            vertexAttributes.emplace_back(
                attribute.location,
                attribute.binding,
                VulkanConvertFormat(attribute.format),
                attribute.offset);
        }

        vk::PipelineVertexInputStateCreateInfo vertexInputState;
        vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindings.size());
        vertexInputState.pVertexBindingDescriptions = vertexBindings.data();
        vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size());
        vertexInputState.pVertexAttributeDescriptions = vertexAttributes.data();

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
        inputAssemblyState.topology = VulkanConvertPrimitiveTopology(desc.topology);

        vk::PipelineViewportStateCreateInfo viewportState;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        vk::PipelineRasterizationStateCreateInfo rasterizationState;
        rasterizationState.depthClampEnable = desc.depthClampEnable;
        rasterizationState.rasterizerDiscardEnable = VK_FALSE;
        rasterizationState.polygonMode = VulkanConvertPolygonMode(desc.polygonMode);
        rasterizationState.cullMode = VulkanConvertCullMode(desc.cullMode);
        rasterizationState.frontFace = VulkanConvertFrontFace(desc.frontFace);
        rasterizationState.depthBiasEnable = desc.depthBiasEnable;
        rasterizationState.lineWidth = 1.0f;

        vk::PipelineMultisampleStateCreateInfo multisampleState;
        multisampleState.rasterizationSamples = VulkanConvertSampleCount(desc.sampleCount);

        vk::PipelineDepthStencilStateCreateInfo depthStencilState;
        depthStencilState.depthTestEnable = desc.depthTestEnable;
        depthStencilState.depthWriteEnable = desc.depthWriteEnable;
        depthStencilState.depthCompareOp = VulkanConvertCompareOp(desc.depthCompareOp);
        depthStencilState.stencilTestEnable = desc.stencilTestEnable;

        std::vector<RHIColorBlendAttachmentDesc> defaultBlendAttachments;
        if (desc.colorBlendAttachments.empty())
        {
            defaultBlendAttachments.resize(desc.colorAttachmentFormats.size());
        }
        const auto &blendAttachmentsSource = desc.colorBlendAttachments.empty()
                                                 ? defaultBlendAttachments
                                                 : desc.colorBlendAttachments;
        if (blendAttachmentsSource.size() != desc.colorAttachmentFormats.size())
        {
            return;
        }

        std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;
        colorBlendAttachments.reserve(blendAttachmentsSource.size());
        for (const auto &attachment: blendAttachmentsSource)
        {
            vk::PipelineColorBlendAttachmentState state;
            state.blendEnable = attachment.blendEnable;
            state.srcColorBlendFactor = VulkanConvertBlendFactor(attachment.srcColorBlendFactor);
            state.dstColorBlendFactor = VulkanConvertBlendFactor(attachment.dstColorBlendFactor);
            state.colorBlendOp = VulkanConvertBlendOp(attachment.colorBlendOp);
            state.srcAlphaBlendFactor = VulkanConvertBlendFactor(attachment.srcAlphaBlendFactor);
            state.dstAlphaBlendFactor = VulkanConvertBlendFactor(attachment.dstAlphaBlendFactor);
            state.alphaBlendOp = VulkanConvertBlendOp(attachment.alphaBlendOp);
            state.colorWriteMask = VulkanConvertColorComponentFlags(attachment.colorWriteMask);
            colorBlendAttachments.push_back(state);
        }

        vk::PipelineColorBlendStateCreateInfo colorBlendState;
        colorBlendState.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
        colorBlendState.pAttachments = colorBlendAttachments.data();

        const vk::DynamicState dynamicStates[] = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
            vk::DynamicState::eBlendConstants,
            vk::DynamicState::eStencilReference
        };
        vk::PipelineDynamicStateCreateInfo dynamicState;
        dynamicState.dynamicStateCount = 4;
        dynamicState.pDynamicStates = dynamicStates;

        std::vector<vk::Format> colorAttachmentFormats;
        colorAttachmentFormats.reserve(desc.colorAttachmentFormats.size());
        for (const auto format: desc.colorAttachmentFormats)
        {
            colorAttachmentFormats.push_back(VulkanConvertFormat(format));
        }

        vk::PipelineRenderingCreateInfo renderingCreateInfo;
        renderingCreateInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentFormats.size());
        renderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
        if (desc.depthStencilFormat != RHIFormat::Undefined)
        {
            const auto depthStencilFormat = VulkanConvertFormat(desc.depthStencilFormat);
            switch (desc.depthStencilFormat)
            {
                case RHIFormat::D32SFloat:
                    renderingCreateInfo.depthAttachmentFormat = depthStencilFormat;
                    renderingCreateInfo.stencilAttachmentFormat = vk::Format::eUndefined;
                    break;
                case RHIFormat::S8Uint:
                    renderingCreateInfo.depthAttachmentFormat = vk::Format::eUndefined;
                    renderingCreateInfo.stencilAttachmentFormat = depthStencilFormat;
                    break;
                case RHIFormat::D32SFloatS8Uint:
                    renderingCreateInfo.depthAttachmentFormat = depthStencilFormat;
                    renderingCreateInfo.stencilAttachmentFormat = depthStencilFormat;
                    break;
                default:
                    renderingCreateInfo.depthAttachmentFormat = depthStencilFormat;
                    renderingCreateInfo.stencilAttachmentFormat = vk::Format::eUndefined;
                    break;
            }
        }

        vk::GraphicsPipelineCreateInfo createInfo;
        createInfo.pNext = &renderingCreateInfo;
        createInfo.stageCount = 2;
        createInfo.pStages = shaderStages;
        createInfo.pVertexInputState = &vertexInputState;
        createInfo.pInputAssemblyState = &inputAssemblyState;
        createInfo.pViewportState = &viewportState;
        createInfo.pRasterizationState = &rasterizationState;
        createInfo.pMultisampleState = &multisampleState;
        createInfo.pDepthStencilState = &depthStencilState;
        createInfo.pColorBlendState = &colorBlendState;
        createInfo.pDynamicState = &dynamicState;
        createInfo.layout = m_ResourceSignature->GetPipelineLayout();

        auto pipelineResult = m_Device.createGraphicsPipeline(VK_NULL_HANDLE, createInfo);
        if (pipelineResult.result != vk::Result::eSuccess || !pipelineResult.value)
        {
            return;
        }

        m_Pipeline = pipelineResult.value;
        m_IsValid = true;
    }

    RHI_VK_FUNC_IMPL(RHIGraphicsPipeline, ~RHIGraphicsPipelineImpl)()
    {
        Release();
    }

    void RHI_VK_FUNC_IMPL(RHIGraphicsPipeline, Release)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto *deviceOwner = m_DeviceOwner;
        ReleaseWithoutUnregister();
        if (deviceOwner)
        {
            deviceOwner->UnregisterGraphicsPipeline(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIGraphicsPipeline, ReleaseImmediate)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto *deviceOwner = m_DeviceOwner;
        ReleaseImmediateWithoutUnregister();
        if (deviceOwner)
        {
            deviceOwner->UnregisterGraphicsPipeline(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIGraphicsPipeline, ReleaseWithoutUnregister)()
    {
        const auto device = m_Device;
        const auto pipeline = m_Pipeline;

        if (m_DeviceOwner)
        {
            m_DeviceOwner->EnqueueDeletion([device, pipeline]()
            {
                if (device && pipeline)
                {
                    device.destroyPipeline(pipeline);
                }
            });
        }
        else if (device && pipeline)
        {
            device.destroyPipeline(pipeline);
        }

        m_Pipeline = VK_NULL_HANDLE;
        m_ResourceSignature = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_DeviceOwner = nullptr;
        m_IsValid = false;
    }

    void RHI_VK_FUNC_IMPL(RHIGraphicsPipeline, ReleaseImmediateWithoutUnregister)()
    {
        if (m_Device && m_Pipeline)
        {
            m_Device.destroyPipeline(m_Pipeline);
        }

        m_Pipeline = VK_NULL_HANDLE;
        m_ResourceSignature = nullptr;
        m_Device = VK_NULL_HANDLE;
        m_DeviceOwner = nullptr;
        m_IsValid = false;
    }
} // Hazel
