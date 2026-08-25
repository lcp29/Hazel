#pragma once

#include "Hazel/Core/Layer.h"
#include "Hazel/Events/ApplicationEvent.h"
// ======== Aster Modify Begin ========
#include "Hazel/RHI/RHI.h"
#include "Hazel/Renderer/GraphicsContext.h"
#include "Hazel/Renderer/Renderer.h"

// ======== Aster Modify End ========

namespace Hazel
{
    class ImGuiLayer : public Layer
    {
      public:
        // ======== Aster Modify Begin ========
        ImGuiLayer(Renderer* renderer);
        ~ImGuiLayer() override = default;
        // ======== Aster Modify End ========

        void OnAttach() override;
        void OnDetach() override;
        void OnEvent(Event& e) override;

        void Begin();
        // ======== Aster Modify Begin ========
        void End(Aster::RHICommandBuffer* commandBuffer = nullptr);

        // ======== Aster Modify End ========

        void BlockEvents(bool block) { m_BlockEvents = block; }

        void SetDarkThemeColors();
        // ======== Aster Modify Begin ========
        void* AddTexture(Aster::RHISampler* sampler,
                         Aster::RHIImageView* imageView,
                         Aster::RHIImageResourceState imageState = Aster::RHIImageResourceState::ShaderRead);
        void RemoveTexture(void* textureID);
        // ======== Aster Modify End ========

        uint32_t GetActiveWidgetID() const;

      private:
        bool m_BlockEvents = true;
        // ======== Aster Modify Begin ========
        Renderer* m_Renderer = nullptr;
        Aster::RHIResourceHeap* m_ResourceHeap = nullptr;
        // ======== Aster Modify End ========
    };
} // namespace Hazel
