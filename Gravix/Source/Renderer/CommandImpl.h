#pragma once

#include "Renderer/Generic/Types/Material.h"
#include "Renderer/Generic/Types/Framebuffer.h"
#include "Renderer/Generic/Types/Mesh.h"
#include "Renderer/Generic/Types/Texture.h"

namespace Gravix 
{

	class CommandImpl 
	{
	public:
		virtual ~CommandImpl() = default;
		
		virtual void SetActiveMaterial(Material* material) = 0;

		virtual void BindResource(uint32_t binding, Framebuffer* buffer, uint32_t index, bool sampler) = 0;
		virtual void BindResource(uint32_t binding, uint32_t index, Texture2D* texture) = 0;
		virtual void BindResource(uint32_t binding, Texture2D* texture) = 0;

		virtual void BindMaterial(void* pushConstants) = 0;
		virtual void Dispatch() = 0;

		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void SetScissor(uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height) = 0;
		virtual void SetLineWidth(float width) = 0;

		virtual void BeginRendering() = 0;
		virtual void BindMesh(Mesh* mesh) = 0;

		virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
		virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;

		virtual void DrawImGui() = 0;

		virtual void ResolveFramebuffer(Framebuffer* dst, bool shaderUse) = 0;

		virtual void EndRendering() = 0;

		virtual void CopyToSwapchain() = 0;
	};

}