#pragma once

#include "Framebuffer.h"
#include "Renderer/CommandImpl.h"

namespace Gravix
{

	class Command
	{
	public:
		Command(Ref<Framebuffer> framebuffer = nullptr, uint32_t presentIndex = 0, bool shouldCopy = true);
		virtual ~Command();

		void BeginRendering();
		void DrawImGui();
		void EndRendering();
	private:
		CommandImpl* m_Impl = nullptr;
	private:
		void Initialize(Ref<Framebuffer> framebuffer, uint32_t presentIndex, bool shouldCopy);
	};
}