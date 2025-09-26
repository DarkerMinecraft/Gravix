#pragma once

#include "Renderer/Generic/Material.h"
#include "Renderer/Generic/Framebuffer.h"

namespace Gravix 
{

	class CommandImpl 
	{
	public:
		virtual ~CommandImpl() = default;
		
		virtual void SetActiveMaterial(Material* material) = 0;
		virtual void BindResource(uint32_t binding, Framebuffer* buffer, uint32_t index, bool sampler) = 0;

		virtual void BindMaterial(void* pushConstants) = 0;
		virtual void Dispatch() = 0;

		virtual void BeginRendering() = 0;
		virtual void DrawImGui() = 0;
		virtual void EndRendering() = 0;

		virtual void CopyToSwapchain() = 0;
	};

}