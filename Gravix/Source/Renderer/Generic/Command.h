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

		void SetActiveMaterial(Material* material);
		void SetActiveMaterial(Ref<Material> material) { SetActiveMaterial(material.get()); }

		void BindResource(uint32_t binding, Framebuffer* buffer, uint32_t index, bool sampler = false);
		void BindResource(uint32_t binding, Ref<Framebuffer> buffer, uint32_t index, bool sampler = false) { BindResource(binding, buffer.get(), index, sampler); }

		void BindMaterial(void* pushConstants = nullptr);
		void Dispatch();

		void BeginRendering();

		void BindMesh(MeshBuffer* mesh);
		void BindMesh(Ref<MeshBuffer> mesh) { BindMesh(mesh.get()); }

		void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);
		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0);

		void DrawImGui();

		void EndRendering();
	private:
		CommandImpl* m_Impl = nullptr;
	private:
		void Initialize(Ref<Framebuffer> framebuffer, uint32_t presentIndex, bool shouldCopy);
	};
}