// TODO: TEMP URGENT INTERVIEW: mesh GPU importer

#include "Hazel/Renderer/GPUAsset/Importer/GPUAssetImporter.h"

#include "Hazel/Asset/MeshAsset.h"
#include "Hazel/Renderer/GraphicsContext.h"
#include "Hazel/Renderer/GPUAsset/GPUMeshAsset.h"
#include "Hazel/Renderer/Renderer.h"

#include <cstring>

namespace Hazel
{
    std::unique_ptr<GPUMeshAsset> ImportGPUMeshAsset(Renderer* renderer, const MeshAsset* asset)
    {
        const auto& meshData = asset->GetData();
        auto* device = renderer->GetDevice();
        auto* queue = device->GetUniformQueue();
        auto* graphicsContext = renderer->GetGraphicsContext();

        auto* cmd = graphicsContext->AcquireDefaultCommandBuffer();
        cmd->Begin(true);

        // TODO: TEMP URGENT INTERVIEW: upload mesh vertex/index data directly into buffers
        RHIBufferDesc vertexStagingDesc{};
        vertexStagingDesc.size = meshData.vertices.size() * sizeof(Vertex);
        vertexStagingDesc.usages = RHIBufferUsageFlagBits::TransferSource;
        vertexStagingDesc.cpuAccess = RHIBufferCpuAccess::Write;
        vertexStagingDesc.mapOnCreate = true;

        auto* vertexStagingBuffer = device->CreateBuffer(vertexStagingDesc, true);
        void* vertexMappedData = vertexStagingBuffer->Map();
        std::memcpy(vertexMappedData, meshData.vertices.data(), vertexStagingDesc.size);
        vertexStagingBuffer->Unmap();

        RHIBufferDesc vertexBufferDesc{};
        vertexBufferDesc.size = vertexStagingDesc.size;
        vertexBufferDesc.usages = RHIBufferUsageFlagBits::TransferDestination | RHIBufferUsageFlagBits::VertexBuffer;
        auto* vertexBuffer = device->CreateBuffer(vertexBufferDesc);

        RHIBufferCopyRegion vertexCopyRegion{};
        vertexCopyRegion.size = vertexBufferDesc.size;
        RHIBufferCopyDesc vertexCopyDesc{};
        vertexCopyDesc.regions.push_back(vertexCopyRegion);
        cmd->CopyBuffer(vertexStagingBuffer, vertexBuffer, vertexCopyDesc);

        RHIBufferDesc indexStagingDesc{};
        indexStagingDesc.size = meshData.indices.size() * sizeof(uint32_t);
        indexStagingDesc.usages = RHIBufferUsageFlagBits::TransferSource;
        indexStagingDesc.cpuAccess = RHIBufferCpuAccess::Write;
        indexStagingDesc.mapOnCreate = true;

        auto* indexStagingBuffer = device->CreateBuffer(indexStagingDesc, true);
        void* indexMappedData = indexStagingBuffer->Map();
        std::memcpy(indexMappedData, meshData.indices.data(), indexStagingDesc.size);
        indexStagingBuffer->Unmap();

        RHIBufferDesc indexBufferDesc{};
        indexBufferDesc.size = indexStagingDesc.size;
        indexBufferDesc.usages = RHIBufferUsageFlagBits::TransferDestination | RHIBufferUsageFlagBits::IndexBuffer;
        auto* indexBuffer = device->CreateBuffer(indexBufferDesc);

        RHIBufferCopyRegion indexCopyRegion{};
        indexCopyRegion.size = indexBufferDesc.size;
        RHIBufferCopyDesc indexCopyDesc{};
        indexCopyDesc.regions.push_back(indexCopyRegion);
        cmd->CopyBuffer(indexStagingBuffer, indexBuffer, indexCopyDesc);

        cmd->End();

        RHIQueueSubmitDesc submitDesc{};
        submitDesc.commandBuffers = {cmd};
        RHISyncPoint syncPoint = queue->Submit(submitDesc);
        device->WaitSyncPoint(&syncPoint);

        vertexStagingBuffer->Release();
        indexStagingBuffer->Release();
        graphicsContext->ReleaseDefaultCommandBuffer(cmd);

        auto meshAsset = std::make_unique<GPUMeshAsset>(asset->GetUUID(),
                                                        asset->GetVersion(),
                                                        renderer,
                                                        meshData.vertices,
                                                        meshData.indices,
                                                        std::vector<GPUMeshletInfo>{},
                                                        renderer->GetCurrentFrameIndex());
        meshAsset->SetVertexBuffer(vertexBuffer);
        meshAsset->SetIndexBuffer(indexBuffer);
        return meshAsset;
    }
} // namespace Hazel
