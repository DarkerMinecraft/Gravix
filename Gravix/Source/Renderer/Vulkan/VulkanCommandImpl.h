#pragma once

#include "Renderer/CommandImpl.h"
#include "Renderer/Generic/Device.h"

#include "VulkanFramebuffer.h"
#include "VulkanMaterial.h"
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

		virtual void BindMaterial(void* pushConstants) override;
		virtual void Dispatch() override;

		virtual void BeginRendering() override;
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