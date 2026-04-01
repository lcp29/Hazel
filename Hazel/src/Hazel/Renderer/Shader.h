//
// Created by helmholtz on 2026/3/31.
//

#pragma once

#include "Material.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/RHI/RHI.h"

namespace Hazel
{
    class Renderer;
    class RenderBuffer;

    class Shader
    {
    public:
        Shader() = delete;

        Shader(UUID uuid, Renderer* renderer, RHIShader* vertexShader, RHIShader* fragmentShader);
        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&& other) noexcept;
        Shader& operator=(Shader&& other) noexcept;
        ~Shader();

        bool IsValid() const
        {
            return m_IsValid;
        }

        RHIShader* GetVertexShader() const
        {
            return m_VertexShader;
        }

        RHIShader* GetFragmentShader() const
        {
            return m_FragmentShader;
        }

        Material* GetMaterial(uint32_t materialID) const
        {
            if (materialID >= m_Materials.size())
            {
                return nullptr;
            }
            return m_Materials[materialID].get();
        }

        uint32_t RegisterMaterial(std::unique_ptr<Material> material);
        void UnregisterMaterial(uint32_t materialID);

        void RecreateMaterialResourceGroup();
        void WriteMaterialResourceGroup();

        RHIResourceSignature* GetResourceSignature() const
        {
            return m_ResourceSignature;
        }

        RHIResourceGroup* GetMaterialResourceGroup() const
        {
            return m_MaterialResourceGroup;
        }

        void Release();
        void ReleaseImmediate();

    private:
        bool m_IsValid = false;
        UUID m_UUID = 0;
        Renderer* m_Renderer = nullptr;
        RHIShader* m_VertexShader = nullptr;
        RHIShader* m_FragmentShader = nullptr;
        std::vector<std::unique_ptr<Material>> m_Materials;
        std::vector<uint32_t> m_MaterialListFreeList;
        std::vector<RHIResourceLayout*> m_ResourceLayouts;
        RHIResourceSignature* m_ResourceSignature = nullptr;
        RHIResourceLayout* m_MaterialResourceLayout = nullptr;
        RHIResourceGroup* m_MaterialResourceGroup = nullptr;
        RHIResourceHeap* m_MaterialResourceHeap = nullptr;
        RenderBuffer* m_MaterialBuffer = nullptr;
        uint32_t m_MaterialCapacity = 0;
        uint32_t m_MaterialBufferStride = 0;
    };
} // namespace Hazel
