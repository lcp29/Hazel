#pragma once

#include "Hazel/Core/Layer.h"

#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/RHI/RHI.h"
#include "Hazel/Renderer/GraphicsContext.h"
#include "Hazel/Renderer/Renderer.h"

namespace Hazel {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer(Renderer *renderer);
		~ImGuiLayer() override = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnEvent(Event& e) override;

		void Begin();
		void End(RHICommandBuffer* commandBuffer = nullptr);

		void BlockEvents(bool block) { m_BlockEvents = block; }
		
		void SetDarkThemeColors();
		void* AddTexture(RHISampler* sampler, RHIImageView* imageView,
			RHIImageResourceState imageState = RHIImageResourceState::ShaderRead);
		void RemoveTexture(void* textureID);

		uint32_t GetActiveWidgetID() const;
	private:
		bool m_BlockEvents = true;
		Renderer *m_Renderer = nullptr;
		RHIResourceHeap *m_ResourceHeap = nullptr;
	};

}
