#pragma once

#include "Renderer/Generic/Types/Framebuffer.h"

namespace Gravix 
{
		class ViewportPanel
		{
		public:
			ViewportPanel() = default;
			ViewportPanel(const Ref<Framebuffer>& framebuffer, uint32_t renderIndex);
			~ViewportPanel() = default;

			void ResizeFramebuffer();
			void OnImGuiRender();

			void SetFramebuffer(const Ref<Framebuffer>& framebuffer, uint32_t renderIndex) { m_Framebuffer = framebuffer; m_RenderIndex = renderIndex; }

			bool IsViewportValid() const { return m_ViewportSize.x > 0 && m_ViewportSize.y > 0; }
			const glm::vec2& GetViewportSize() const { return m_ViewportSize; }
		private:
			Ref<Framebuffer> m_Framebuffer;
			glm::vec2 m_ViewportSize = { 1280.0f, 720.0f };

			uint32_t m_RenderIndex = 0;
		};
}