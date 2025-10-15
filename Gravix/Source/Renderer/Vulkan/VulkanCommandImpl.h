#pragma once

#include "Renderer/CommandImpl.h"
#include "Renderer/Generic/Device.h"

#include "Types/VulkanFramebuffer.h"
#include "Types/VulkanMaterial.h"
#include "VulkanDevice.h"

namespace Gravix 
{

	class VulkanCommandImpl : public CommandImpl
	{
	public:
		VulkanCommandImpl(Device* device, Ref<Framebuffer> targetFrameBuffer, uint32_t presentIndex, bool shouldCopy);
		virtual ~VulkanCommandImpl();

		virtual void SetActiveMaterial(Material* material) override { m_BoundMaterial = static_cast<VulkanMaterial*>(material); }

		virtual void BindResource(uint32_t binding, Framebuffer* buffer, uint32_t index, bool sampler) override;
		virtual void BindResource(uint32_t binding, uint32_t index, Texture2D* texture) override;
		virtual void BindResource(uint32_t binding, Texture2D* texture) override { BindResource(binding, 0, texture); }

		virtual void BindMaterial(void* pushConstants) override;
		virtual void Dispatch() override;

		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		virtual void SetScissor(uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height) override;

		virtual void BeginRendering() override;

		virtual void BindMesh(Mesh* mesh) override;

		virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
		virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override;

		virtual void DrawImGui() override;
		virtual void EndRendering() override;

		virtual void CopyToSwapchain() override;
	private:
		VulkanDevice* m_Device;
		VkCommandBuffer m_CommandBuffer;

		VulkanFramebuffer* m_TargetFramebuffer;
		VulkanMaterial* m_BoundMaterial = nullptr;

		uint32_t m_PresentIndex = 0;
		bool m_ShouldCopy = false; 
	};

}