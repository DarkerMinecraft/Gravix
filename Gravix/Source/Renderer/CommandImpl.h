#pragma once

#include "Renderer/Generic/Material.h"
#include "Renderer/Generic/Framebuffer.h"
#include "Renderer/Generic/MeshBuffer.h"

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
		virtual void BindMesh(MeshBuffer* mesh) = 0;

		virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
		virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);

		virtual void DrawImGui() = 0;

		virtual void EndRendering() = 0;

		virtual void CopyToSwapchain() = 0;
	};

}