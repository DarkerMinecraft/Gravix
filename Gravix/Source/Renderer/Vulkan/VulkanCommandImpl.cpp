#include "pch.h"
#include "VulkanCommandImpl.h"

#include "VulkanDevice.h"

#include "Types/VulkanMesh.h"

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
			if (m_TargetFramebuffer == nullptr) return;

			m_TargetFramebuffer->TransitionToLayout(m_CommandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			m_TargetFramebuffer->TransitionDepthToShaderRead(m_CommandBuffer);
		}
	}

	void VulkanCommandImpl::BindResource(uint32_t binding, Framebuffer* buffer, uint32_t index, bool sampler)
	{
		if (m_BoundMaterial != nullptr && buffer != nullptr)
			m_BoundMaterial->BindResource(m_CommandBuffer, binding, buffer, index, sampler);
	}

	void VulkanCommandImpl::BindResource(uint32_t binding, uint32_t index, Texture2D* texture)
	{
		if(m_BoundMaterial != nullptr && texture != nullptr)
			m_BoundMaterial->BindResource(m_CommandBuffer, binding, index, texture);
	}

	void VulkanCommandImpl::BindMaterial(void* pushConstants)
	{
		if (m_BoundMaterial != nullptr)
			m_BoundMaterial->Bind(m_CommandBuffer, pushConstants);
	}

	void VulkanCommandImpl::Dispatch()
	{
		if (m_BoundMaterial != nullptr && m_TargetFramebuffer != nullptr)
			m_BoundMaterial->Dispatch(m_CommandBuffer, m_TargetFramebuffer->GetWidth(), m_TargetFramebuffer->GetHeight());
	}

	void VulkanCommandImpl::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		VkViewport viewport{};
		viewport.x = x;
		viewport.y = y;
		viewport.width = static_cast<float>(width);
		viewport.height = static_cast<float>(height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
	}

	void VulkanCommandImpl::SetScissor(uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height)
	{
		VkRect2D scissor{};
		scissor.offset = { static_cast<int32_t>(offsetX), static_cast<int32_t>(offsetY) };
		scissor.extent = { width, height };

		vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);
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

			VkExtent2D extent = m_Device->GetSwapchainExtent();
			SetViewport(0, 0, extent.width, extent.height);
			SetScissor(0, 0, extent.width, extent.height);
		}
		else
		{
			std::vector<VkRenderingAttachmentInfo> colorAttachments = m_TargetFramebuffer->GetColorAttachments();
			VkRenderingAttachmentInfo* depthAttachment = m_TargetFramebuffer->GetDepthAttachment();

			VkRenderingInfo renderInfo = VulkanInitializers::RenderingInfo({ m_TargetFramebuffer->GetWidth(), m_TargetFramebuffer->GetHeight() }, colorAttachments, depthAttachment);

			m_TargetFramebuffer->TransitionToBeginRendering(m_CommandBuffer);
			vkCmdBeginRendering(m_CommandBuffer, &renderInfo);

			SetViewport(0, 0, m_TargetFramebuffer->GetWidth(), m_TargetFramebuffer->GetHeight());
			SetScissor(0, 0, m_TargetFramebuffer->GetWidth(), m_TargetFramebuffer->GetHeight());
		}
	}

	void VulkanCommandImpl::BindMesh(Mesh* mesh)
	{
		VulkanMesh* vulkanMesh = static_cast<VulkanMesh*>(mesh);
		if (vulkanMesh == nullptr)
			return;
		vulkanMesh->Bind(m_CommandBuffer);
	}

	void VulkanCommandImpl::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void VulkanCommandImpl::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
	{
		vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
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

	void VulkanCommandImpl::ResolveFramebuffer(Framebuffer* dst, bool shaderUse)
	{
		if(m_TargetFramebuffer == nullptr || dst == nullptr)
			return;

		VulkanFramebuffer* vulkanDst = static_cast<VulkanFramebuffer*>(dst);

		m_TargetFramebuffer->TransitionToLayout(m_CommandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		vulkanDst->TransitionToLayout(m_CommandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		for(uint32_t i = 0; i < m_TargetFramebuffer->GetAttachments().size(); i++)
		{
			if(i == m_TargetFramebuffer->GetDepthAttachmentIndex())
				continue;

			VulkanUtils::ResolveImage(m_CommandBuffer, m_TargetFramebuffer->GetImage(i).Image, vulkanDst->GetImage(i).Image, { m_TargetFramebuffer->GetWidth(), m_TargetFramebuffer->GetHeight() });
		}

		if (shaderUse)
			vulkanDst->TransitionToLayout(m_CommandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		else 
			vulkanDst->TransitionToLayout(m_CommandBuffer, VK_IMAGE_LAYOUT_GENERAL);

		m_TargetFramebuffer->TransitionToLayout(m_CommandBuffer, VK_IMAGE_LAYOUT_GENERAL);
	}

	void VulkanCommandImpl::CopyToSwapchain()
	{
		if (m_TargetFramebuffer == nullptr)
			return;

		m_TargetFramebuffer->TransitionToLayout(m_CommandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		VkImageLayout swapchainImageLayout = m_Device->GetCurrentSwapchainImageLayout();
		if (swapchainImageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			VulkanUtils::TransitionImage(m_CommandBuffer, m_Device->GetCurrentSwapchainImage(), m_Device->GetSwapchainImageFormat(), swapchainImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VulkanUtils::CopyImageToImage(m_CommandBuffer, m_TargetFramebuffer->GetImage(m_PresentIndex).Image, m_Device->GetCurrentSwapchainImage(), { m_TargetFramebuffer->GetWidth(), m_TargetFramebuffer->GetHeight() }, m_Device->GetSwapchainExtent());

		VulkanUtils::TransitionImage(m_CommandBuffer, m_Device->GetCurrentSwapchainImage(), m_Device->GetSwapchainImageFormat(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, swapchainImageLayout);

		m_TargetFramebuffer->TransitionToLayout(m_CommandBuffer, VK_IMAGE_LAYOUT_GENERAL);
	}

}