//
// Created by helmholtz on 2026/4/7.
//

#include "GeometryDataRegistry.h"

#include "Renderer.h"

namespace Hazel
{
    GeometryDataRegistry::GeometryDataRegistry(Renderer* renderer)
        : m_Renderer(renderer)
    {
        RHIBufferDesc pageTableDesc{};
        pageTableDesc.size = kMaxPageCount * sizeof(GPUPageTerm);
        pageTableDesc.usages = RHIBufferUsageFlagBits::StorageBuffer;
        pageTableDesc.allowGpuAddress = false;
        pageTableDesc.cpuAccess = RHIBufferCpuAccess::Write;
        pageTableDesc.hostCoherent = true;
        pageTableDesc.mapOnCreate = true;
        m_PhysicalPageTable = m_Renderer->GetDevice()->CreateBuffer(pageTableDesc);

        for (uint32_t index = 0; index < kMaxPageCount; index++)
        {
            m_FreeVirtualPageIndices.push_back(index);
        }

        CreateNewPageBuffer();
    }

    GeometryDataRegistry::~GeometryDataRegistry()
    {
        for (auto* buffer : m_PageBuffers)
        {
            if (buffer) { buffer->Release(); }
        }
        m_PageBuffers.clear();
        if (m_PhysicalPageTable) { m_PhysicalPageTable->Release(); }
    }

    void GeometryDataRegistry::RegisterMesh(GPUMeshAsset* meshAsset)
    {
        uint32_t vertexSize = meshAsset->GetVertices().size() * kGPUVertexSize;
        uint32_t indexSize = meshAsset->GetIndices().size() * sizeof(uint32_t);

        uint32_t vertexPageCount = (vertexSize + kPageSize - 1) / kPageSize;
        uint32_t indexPageCount = (indexSize + kPageSize - 1) / kPageSize;

        std::vector<uint32_t> vertexVirtualPages;
        std::vector<uint32_t> indexVirtualPages;

        std::unique_lock lock(m_VirtualPageTableMutex);
        for (uint32_t i = 0; i < vertexPageCount; i++)
        {
            auto newVirtualPageIndex = m_FreeVirtualPageIndices.back();
            m_FreeVirtualPageIndices.pop_back();
            vertexVirtualPages.push_back(newVirtualPageIndex);
        }

        for (uint32_t i = 0; i < indexPageCount; i++)
        {
            auto newVirtualPageIndex = m_FreeVirtualPageIndices.back();
            m_FreeVirtualPageIndices.pop_back();
            indexVirtualPages.push_back(newVirtualPageIndex);
        }
        lock.unlock();

        meshAsset->SetVertexVirtualPages(std::move(vertexVirtualPages));
        meshAsset->SetIndexVirtualPages(std::move(indexVirtualPages));

        if (meshAsset->HasMeshlets())
        {
            std::vector<uint32_t> meshletVirtualPages;

            uint32_t meshletSize = meshAsset->GetMeshlets().size() * sizeof(GPUMeshletInfo);
            uint32_t meshletPageCount = (meshletSize + kPageSize - 1) / kPageSize;

            lock.lock();
            for (uint32_t i = 0; i < meshletPageCount; i++)
            {
                auto newVirtualPageIndex = m_FreeVirtualPageIndices.back();
                m_FreeVirtualPageIndices.pop_back();
                meshletVirtualPages.push_back(newVirtualPageIndex);
            }
            lock.unlock();

            meshAsset->SetMeshletVirtualPages(std::move(meshletVirtualPages));
        }
    }

    void GeometryDataRegistry::UnregisterMesh(GPUMeshAsset* meshAsset)
    {
        std::unique_lock lock(m_VirtualPageTableMutex);
        for (auto virtualPageIndex : meshAsset->GetVertexVirtualPages())
        {
            m_FreeVirtualPageIndices.push_back(virtualPageIndex);
        }
        for (auto virtualPageIndex : meshAsset->GetIndexVirtualPages())
        {
            m_FreeVirtualPageIndices.push_back(virtualPageIndex);
        }
        if (meshAsset->HasMeshlets())
        {
            for (auto virtualPageIndex : meshAsset->GetMeshletVirtualPages())
            {
                m_FreeVirtualPageIndices.push_back(virtualPageIndex);
            }
        }
    }

    void GeometryDataRegistry::CreateNewPageBuffer()
    {
        RHIBufferDesc pageBufferDesc{};
        pageBufferDesc.size = kPhysicalBufferSize;
        pageBufferDesc.usages = RHIBufferUsageFlagBits::StorageBuffer | RHIBufferUsageFlagBits::TransferDestination;
        pageBufferDesc.allowGpuAddress = true;
        pageBufferDesc.cpuAccess = RHIBufferCpuAccess::None;
        pageBufferDesc.hostCoherent = false;
        pageBufferDesc.mapOnCreate = false;

        auto* newPageBuffer = m_Renderer->GetDevice()->CreateBuffer(pageBufferDesc);
        m_PageBuffers.push_back(newPageBuffer);
        m_PageBufferMutexes.push_back(std::make_unique<std::mutex>());
        m_PageBufferFreeRanges.push_back({{0, kMaxPageCountPerBuffer}});
    }
} // namespace Hazel