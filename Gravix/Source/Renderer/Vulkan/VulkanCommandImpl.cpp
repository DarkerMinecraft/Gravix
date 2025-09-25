#include "pch.h"
#include "VulkanCommandImpl.h"

#include "VulkanDevice.h"

#include "Utils/VulkanInitializers.h"
#include "Utils/VulkanUtils.h"

#include <backends/imgui_impl_vulkan.h>

namespace Gravix 
{

	VulkanCommandImpl::VulkanCommandImpl(Device* device, Ref<Framebuffer> targetFrameBuffer, uint32_t presentIndex, bool shouldCopy)
		: m_Device(static_cast<VulkanDevice*>(device)), m_CommandBuffer(static_cast<VulkanDevice*>(device)->GetCurrentFrameData().CommandBuffer), m_TargetFramebuffer(static_cast<VulkanFramebuffer*>(targetFrameBuffer.get())), 
		m_PresentIndex(presentIndex), m_ShouldCopy(shouldCopy)
	{
		if (m_TargetFramebuffer != nullptr)
			m_TargetFramebuffer->StartFramebuffer(m_CommandBuffer);
	}

	VulkanCommandImpl::~VulkanCommandImpl()
	{
		if (m_ShouldCopy)
			CopyToSwapchain();
		else 
		{
			if (m_TargetFramebuffer != nullptr)
				m_TargetFramebuffer->TransitionToLayout(m_CommandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}

	void VulkanCommandImpl::BeginRendering()
	{
		if (m_TargetFramebuffer == nullptr)
		{
			VkClearValue clearValue{};
			clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };

			VkRenderingAttachmentInfo colorAttachment = VulkanInitializers::AttachmentInfo(m_Device->GetCurrentSwapchainImageView(), &clearValue);
			VkRenderingInfo renderInfo = VulkanInitializers::RenderingInfo(m_Device->GetSwapchainExtent(), &colorAttachment);

			vkCmdBeginRendering(m_CommandBuffer, &renderInfo);
		}
		else 
		{
			std::vector<VkRenderingAttachmentInfo> colorAttachments = m_TargetFramebuffer->GetColorAttachments();
			VkRenderingAttachmentInfo* depthAttachment = m_TargetFramebuffer->GetDepthAttachment();

			VkRenderingInfo renderInfo = VulkanInitializers::RenderingInfo({ m_TargetFramebuffer->GetWidth(), m_TargetFramebuffer->GetHeight() }, colorAttachments, depthAttachment);

			m_TargetFramebuffer->TransitionToBeginRendering(m_CommandBuffer);
			vkCmdBeginRendering(m_CommandBuffer, &renderInfo);
		}
	}

	void VulkanCommandImpl::DrawImGui()
	{
		ImGui::Render();
		ImDrawData* drawData = ImGui::GetDrawData();
		if (drawData && drawData->TotalVtxCount > 0)
		{
			ImGui_ImplVulkan_RenderDrawData(drawData, m_CommandBuffer);
		}
	}

	void VulkanCommandImpl::EndRendering()
	{
		vkCmdEndRendering(m_CommandBuffer);
	}

	void VulkanCommandImpl::CopyToSwapchain()
	{
		if(m_TargetFramebuffer == nullptr)
			return;

		m_TargetFramebuffer->TransitionToLayout(m_CommandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		VkImageLayout swapchainImageLayout = m_Device->GetCurrentSwapchainImageLayout();
		if (swapchainImageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			VulkanUtils::TransitionImage(m_CommandBuffer, m_Device->GetCurrentSwapchainImage(), swapchainImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		
		VulkanUtils::CopyImageToImage(m_CommandBuffer, m_TargetFramebuffer->GetImage(m_PresentIndex).Image, m_Device->GetCurrentSwapchainImage(), { m_TargetFramebuffer->GetWidth(), m_TargetFramebuffer->GetHeight() }, m_Device->GetSwapchainExtent());

		VulkanUtils::TransitionImage(m_CommandBuffer, m_Device->GetCurrentSwapchainImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, swapchainImageLayout);

		m_TargetFramebuffer->TransitionToLayout(m_CommandBuffer, VK_IMAGE_LAYOUT_GENERAL);
	}

}