#pragma once

#include "Renderer/CommandImpl.h"
#include "Renderer/Generic/Device.h"

#include "VulkanFramebuffer.h"

#include <vulkan/vulkan.h>

namespace Gravix 
{

	class VulkanCommandImpl : public CommandImpl
	{
	public:
		VulkanCommandImpl(Device* device, Ref<Framebuffer> targetFrameBuffer, uint32_t presentIndex, bool shouldCopy);
		virtual ~VulkanCommandImpl();

		virtual void BeginRendering() override;
		virtual void DrawImGui() override;
		virtual void EndRendering() override;

		virtual void CopyToSwapchain() override;
	private:
		VulkanDevice* m_Device;
		VkCommandBuffer m_CommandBuffer;

		VulkanFramebuffer* m_TargetFramebuffer;
		uint32_t m_PresentIndex = 0;
		bool m_ShouldCopy = false; 
	};

}