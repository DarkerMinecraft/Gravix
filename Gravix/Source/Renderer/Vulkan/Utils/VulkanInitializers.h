#pragma once

#include <vulkan/vulkan.h>

namespace Gravix 
{

	class VulkanInitializers
	{
	public:
		static VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags /*= 0*/);
		static VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

		static VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
		static VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t count);

		static VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);

		static VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd);
		static VkSemaphoreSubmitInfo SemaphoreSubmitInfo(VkPipelineStageFlags2 stage, VkSemaphore semaphore);
		static VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
			VkSemaphoreSubmitInfo* waitSemaphoreInfo);

		static VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkSampleCountFlagBits samples, VkExtent3D extent);
		static VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

		static VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask, uint32_t baseMipLevel = 0);

		static VkRenderingAttachmentInfo AttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		static VkRenderingInfo RenderingInfo(VkExtent2D extent, const std::vector<VkRenderingAttachmentInfo>& colorAttachments, VkRenderingAttachmentInfo* depthAttachment = nullptr);
		static VkRenderingInfo RenderingInfo(VkExtent2D extent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment = nullptr);
	};

}