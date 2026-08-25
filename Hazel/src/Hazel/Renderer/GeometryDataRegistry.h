// Declares virtualized GPU geometry storage.
// Created: 2026-04-07.

#pragma once
#include "GPUAsset/GPUMeshAsset.h"

#include <cstdint>

namespace Hazel
{
    class Renderer;
}

namespace Aster
{

    // 4GB
    constexpr uint64_t kGeometrySpaceSize = static_cast<uint64_t>(1024) * 1024 * 1024 * 4;
    // 16KB
    constexpr uint64_t kPageSize = 16384;
    // 256K pages
    constexpr uint32_t kMaxPageCount = static_cast<uint32_t>(kGeometrySpaceSize / kPageSize);

    constexpr uint64_t kVertexCountPerPage = kPageSize / kGPUVertexSize;

    constexpr uint64_t kPhysicalBufferSize = 256 * 1024 * 1024;

    constexpr uint32_t kMaxPageCountPerBuffer = static_cast<uint32_t>(kPhysicalBufferSize / kPageSize);

    struct alignas(16) GPUPageTerm
    {
        uint32_t GetPageIndex() const { return pageIndex; }

        void SetPageIndex(uint32_t index) { pageIndex = index; }

        uint32_t GetBufferIndex() const { return GetPageIndex() / kMaxPageCountPerBuffer; }

        uint32_t GetPageLocalIndexInBuffer() const { return GetPageIndex() % kMaxPageCountPerBuffer; }

      private:
        uint32_t pageIndex = 0;
    };

    class GeometryDataRegistry
    {
      public:
        GeometryDataRegistry() = delete;

        GeometryDataRegistry(Hazel::Renderer* renderer);
        ~GeometryDataRegistry();

        void RegisterMesh(GPUMeshAsset* meshAsset);
        void UnregisterMesh(GPUMeshAsset* meshAsset);

      private:
        void CreateNewPageBuffer();

        Hazel::Renderer* m_Renderer = nullptr;

        std::vector<RHIBuffer*> m_PageBuffers;
        std::vector<std::unique_ptr<std::mutex>> m_PageBufferMutexes;
        std::vector<std::map<uint32_t, uint32_t>> m_PageBufferFreeRanges;

        std::mutex m_VirtualPageTableMutex;
        RHIBuffer* m_PhysicalPageTable = nullptr;
        std::vector<uint32_t> m_FreeVirtualPageIndices;
        std::array<GPUPageTerm, kMaxPageCount> m_VirtualPageTable;
    };
} // namespace Aster
