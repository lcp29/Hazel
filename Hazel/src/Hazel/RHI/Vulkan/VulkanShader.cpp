//
// Created by helmholtz on 2026/3/15.
//

#define VULKAN_HPP_NO_EXCEPTIONS

#include "VulkanShader.h"
#include "VulkanDevice.h"

#include <algorithm>
#include <spirv_cross/spirv_cross.hpp>

namespace Hazel
{
    namespace
    {
        RHIShaderValueBaseType VulkanConvertShaderValueBaseType(const spirv_cross::SPIRType &type)
        {
            switch (type.basetype)
            {
                case spirv_cross::SPIRType::Boolean:
                    return RHIShaderValueBaseType::Boolean;
                case spirv_cross::SPIRType::Int:
                case spirv_cross::SPIRType::Int64:
                case spirv_cross::SPIRType::Short:
                case spirv_cross::SPIRType::SByte:
                    return RHIShaderValueBaseType::SInt;
                case spirv_cross::SPIRType::UInt:
                case spirv_cross::SPIRType::UInt64:
                case spirv_cross::SPIRType::UShort:
                case spirv_cross::SPIRType::UByte:
                    return RHIShaderValueBaseType::UInt;
                case spirv_cross::SPIRType::Half:
                case spirv_cross::SPIRType::Float:
                case spirv_cross::SPIRType::Double:
                    return RHIShaderValueBaseType::Float;
                case spirv_cross::SPIRType::Struct:
                    return RHIShaderValueBaseType::Struct;
                default:
                    return RHIShaderValueBaseType::Unknown;
            }
        }

        std::string GetScalarTypeName(const spirv_cross::SPIRType &type)
        {
            switch (type.basetype)
            {
                case spirv_cross::SPIRType::Boolean:
                    return "bool";
                case spirv_cross::SPIRType::Int:
                    return "int";
                case spirv_cross::SPIRType::UInt:
                    return "uint";
                case spirv_cross::SPIRType::Float:
                    return "float";
                case spirv_cross::SPIRType::Double:
                    return "double";
                case spirv_cross::SPIRType::Half:
                    return "half";
                case spirv_cross::SPIRType::Int64:
                    return "int64_t";
                case spirv_cross::SPIRType::UInt64:
                    return "uint64_t";
                case spirv_cross::SPIRType::Short:
                    return "short";
                case spirv_cross::SPIRType::UShort:
                    return "ushort";
                case spirv_cross::SPIRType::SByte:
                    return "int8_t";
                case spirv_cross::SPIRType::UByte:
                    return "uint8_t";
                case spirv_cross::SPIRType::Struct:
                    return "struct";
                default:
                    return "unknown";
            }
        }

        std::string GetTypeName(spirv_cross::Compiler &compiler, const spirv_cross::SPIRType &type)
        {
            if (type.basetype == spirv_cross::SPIRType::Struct)
            {
                const auto name = compiler.get_name(type.self);
                return name.empty() ? "struct" : name;
            }

            std::string typeName = GetScalarTypeName(type);
            if (type.columns > 1)
            {
                typeName += "mat" + std::to_string(type.columns) + "x" + std::to_string(type.vecsize);
            }
            else if (type.vecsize > 1)
            {
                typeName += "vec" + std::to_string(type.vecsize);
            }

            if (!type.array.empty())
            {
                for (const auto extent: type.array)
                {
                    typeName += "[" + std::to_string(extent) + "]";
                }
            }

            return typeName;
        }

        uint32_t GetArraySize(const spirv_cross::SPIRType &type)
        {
            if (type.array.empty())
            {
                return 0;
            }

            uint32_t totalArraySize = 1;
            for (const auto extent: type.array)
            {
                totalArraySize *= extent;
            }
            return totalArraySize;
        }

        RHIResourceBindingType GetBindingType(spirv_cross::Compiler &compiler,
                                              const spirv_cross::Resource &resource,
                                              const spirv_cross::SPIRType &type,
                                              const bool storage)
        {
            if (type.image.dim == spv::DimBuffer)
            {
                return storage ? RHIResourceBindingType::StorageTexelBuffer
                               : RHIResourceBindingType::UniformTexelBuffer;
            }

            if (type.basetype == spirv_cross::SPIRType::Sampler)
            {
                return RHIResourceBindingType::Sampler;
            }

            if (type.basetype == spirv_cross::SPIRType::Image)
            {
                return storage ? RHIResourceBindingType::StorageImage
                               : RHIResourceBindingType::SampledImage;
            }

            if (type.basetype == spirv_cross::SPIRType::SampledImage)
            {
                return RHIResourceBindingType::SamplerWithImage;
            }

            if (compiler.has_decoration(resource.id, spv::DecorationNonWritable) && storage)
            {
                return RHIResourceBindingType::StorageBuffer;
            }

            return storage ? RHIResourceBindingType::StorageBuffer
                           : RHIResourceBindingType::UniformBuffer;
        }

        RHIShaderResourceGroupReflection &GetOrCreateResourceGroup(RHIShaderReflection &reflection, const uint32_t set)
        {
            for (auto &resourceGroup: reflection.resourceGroups)
            {
                if (resourceGroup.set == set)
                {
                    return resourceGroup;
                }
            }

            reflection.resourceGroups.push_back({set});
            return reflection.resourceGroups.back();
        }

        void SortReflection(RHIShaderReflection &reflection)
        {
            std::sort(reflection.resourceGroups.begin(),
                      reflection.resourceGroups.end(),
                      [](const auto &lhs, const auto &rhs)
                      {
                          return lhs.set < rhs.set;
                      });

            for (auto &resourceGroup: reflection.resourceGroups)
            {
                std::sort(resourceGroup.bindings.begin(),
                          resourceGroup.bindings.end(),
                          [](const auto &lhs, const auto &rhs)
                          {
                              return lhs.binding < rhs.binding;
                          });
            }
        }

        RHIShaderBufferReflection ReflectBuffer(spirv_cross::Compiler &compiler,
                                                const spirv_cross::Resource &resource)
        {
            RHIShaderBufferReflection reflection;
            reflection.name = resource.name;

            const auto &bufferType = compiler.get_type(resource.base_type_id);
            reflection.size = static_cast<uint32_t>(compiler.get_declared_struct_size(bufferType));
            reflection.members.reserve(bufferType.member_types.size());

            for (uint32_t memberIndex = 0; memberIndex < static_cast<uint32_t>(bufferType.member_types.size());
                 memberIndex++)
            {
                const auto &memberType = compiler.get_type(bufferType.member_types[memberIndex]);

                RHIShaderBufferMemberReflection memberReflection;
                memberReflection.name = compiler.get_member_name(resource.base_type_id, memberIndex);
                memberReflection.typeName = GetTypeName(compiler, memberType);
                memberReflection.baseType = VulkanConvertShaderValueBaseType(memberType);
                memberReflection.columns = memberType.columns;
                memberReflection.rows = memberType.vecsize;
                memberReflection.arraySize = GetArraySize(memberType);
                memberReflection.size = static_cast<uint32_t>(
                    compiler.get_declared_struct_member_size(bufferType, memberIndex));
                memberReflection.offset = compiler.type_struct_member_offset(bufferType, memberIndex);
                reflection.members.push_back(std::move(memberReflection));
            }

            return reflection;
        }

        void ReflectBoundResource(RHIShaderReflection &reflection,
                                  spirv_cross::Compiler &compiler,
                                  const spirv_cross::Resource &resource,
                                  const RHIResourceBindingType bindingType,
                                  const bool reflectBuffer)
        {
            const auto set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            const auto binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

            auto &resourceGroup = GetOrCreateResourceGroup(reflection, set);
            auto &bindingReflection = resourceGroup.bindings.emplace_back();
            bindingReflection.binding = binding;
            bindingReflection.type = bindingType;
            bindingReflection.count = 1;

            const auto &resourceType = compiler.get_type(resource.type_id);
            if (!resourceType.array.empty())
            {
                bindingReflection.count = GetArraySize(resourceType);
            }

            if (!resource.name.empty())
            {
                bindingReflection.variableName = resource.name;
            }

            if (reflectBuffer)
            {
                bindingReflection.buffer = ReflectBuffer(compiler, resource);
            }
        }
    } // namespace

    RHI_VK_FUNC_IMPL(RHIShader, RHIShaderImpl)(RHIDevice *deviceOwner, vk::Device device, const RHIShaderDesc &desc)
    {
        m_DeviceOwner = deviceOwner;
        m_Device = device;
        m_Desc = desc;

        if (!m_DeviceOwner || !m_Device || desc.binary.empty() || desc.entryPoint.empty())
        {
            return;
        }

        vk::ShaderModuleCreateInfo createInfo;
        createInfo.codeSize = desc.binary.size() * sizeof(uint32_t);
        createInfo.pCode = desc.binary.data();

        auto createResult = m_Device.createShaderModule(createInfo);
        if (createResult.result != vk::Result::eSuccess || !createResult.value)
        {
            return;
        }

        m_ShaderModule = createResult.value;
        if (!Reflect())
        {
            m_Device.destroyShaderModule(m_ShaderModule);
            m_ShaderModule = VK_NULL_HANDLE;
            return;
        }

        m_IsValid = true;
    }

    RHI_VK_FUNC_IMPL(RHIShader, ~RHIShaderImpl)()
    {
        Release();
    }

    void RHI_VK_FUNC_IMPL(RHIShader, Release)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto *deviceOwner = m_DeviceOwner;
        ReleaseWithoutUnregister();
        if (deviceOwner)
        {
            deviceOwner->UnregisterShader(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIShader, ReleaseImmediate)()
    {
        if (!m_IsValid)
        {
            return;
        }

        auto *deviceOwner = m_DeviceOwner;
        ReleaseImmediateWithoutUnregister();
        if (deviceOwner)
        {
            deviceOwner->UnregisterShader(this);
        }
    }

    void RHI_VK_FUNC_IMPL(RHIShader, ReleaseWithoutUnregister)()
    {
        const auto device = m_Device;
        const auto shaderModule = m_ShaderModule;

        if (m_DeviceOwner)
        {
            m_DeviceOwner->EnqueueDeletion([device, shaderModule]()
            {
                if (device && shaderModule)
                {
                    device.destroyShaderModule(shaderModule);
                }
            });
        }
        else if (device && shaderModule)
        {
            device.destroyShaderModule(shaderModule);
        }

        m_ShaderModule = VK_NULL_HANDLE;
        m_Device = VK_NULL_HANDLE;
        m_DeviceOwner = nullptr;
        m_IsValid = false;
        m_Reflection = {};
    }

    void RHI_VK_FUNC_IMPL(RHIShader, ReleaseImmediateWithoutUnregister)()
    {
        if (m_Device && m_ShaderModule)
        {
            m_Device.destroyShaderModule(m_ShaderModule);
        }

        m_ShaderModule = VK_NULL_HANDLE;
        m_Device = VK_NULL_HANDLE;
        m_DeviceOwner = nullptr;
        m_IsValid = false;
        m_Reflection = {};
    }

    bool RHI_VK_FUNC_IMPL(RHIShader, Reflect)()
    {
        spirv_cross::Compiler compiler(m_Desc.binary);
        const auto resources = compiler.get_shader_resources();

        for (const auto &resource: resources.uniform_buffers)
        {
            ReflectBoundResource(m_Reflection, compiler, resource, RHIResourceBindingType::UniformBuffer, true);
        }

        for (const auto &resource: resources.storage_buffers)
        {
            ReflectBoundResource(m_Reflection, compiler, resource, RHIResourceBindingType::StorageBuffer, true);
        }

        for (const auto &resource: resources.separate_samplers)
        {
            ReflectBoundResource(m_Reflection, compiler, resource, RHIResourceBindingType::Sampler, false);
        }

        for (const auto &resource: resources.sampled_images)
        {
            const auto &type = compiler.get_type(resource.type_id);
            ReflectBoundResource(
                m_Reflection,
                compiler,
                resource,
                GetBindingType(compiler, resource, type, false),
                false);
        }

        for (const auto &resource: resources.separate_images)
        {
            const auto &type = compiler.get_type(resource.type_id);
            ReflectBoundResource(
                m_Reflection,
                compiler,
                resource,
                GetBindingType(compiler, resource, type, false),
                false);
        }

        for (const auto &resource: resources.storage_images)
        {
            const auto &type = compiler.get_type(resource.type_id);
            ReflectBoundResource(
                m_Reflection,
                compiler,
                resource,
                GetBindingType(compiler, resource, type, true),
                false);
        }

        for (const auto &resource: resources.push_constant_buffers)
        {
            auto &pushConstantReflection = m_Reflection.pushConstants.emplace_back();
            pushConstantReflection.name = resource.name;
            auto bufferReflection = ReflectBuffer(compiler, resource);
            pushConstantReflection.size = bufferReflection.size;
            pushConstantReflection.offset = 0;
            pushConstantReflection.members = std::move(bufferReflection.members);
        }

        SortReflection(m_Reflection);
        return true;
    }
} // Hazel
