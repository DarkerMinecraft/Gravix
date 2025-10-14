#include "pch.h"
#include "VulkanInitializers.h"

namespace Gravix
{

	VkFenceCreateInfo VulkanInitializers::FenceCreateInfo(VkFenceCreateFlags flags /*= 0*/)
	{
		VkFenceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = flags;

		return info;
	}

	VkSemaphoreCreateInfo VulkanInitializers::SemaphoreCreateInfo(VkSemaphoreCreateFlags flags /*= 0*/)
	{
		VkSemaphoreCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = flags;

		return info;
	}

	VkCommandPoolCreateInfo VulkanInitializers::CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags /*= 0*/)
	{
		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.pNext = nullptr;
		info.queueFamilyIndex = queueFamilyIndex;
		info.flags = flags;

		return info;
	}

	VkCommandBufferAllocateInfo VulkanInitializers::CommandBufferAllocateInfo(VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t count)
	{
		VkCommandBufferAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.pNext = nullptr;
		info.commandPool = commandPool;
		info.commandBufferCount = 1;
		info.level = level;

		return info;
	}

	VkCommandBufferBeginInfo VulkanInitializers::CommandBufferBeginInfo(VkCommandBufferUsageFlags flags /*= 0*/)
	{
		VkCommandBufferBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.pNext = nullptr;

		info.pInheritanceInfo = nullptr;
		info.flags = flags;
		return info;
	}

	VkCommandBufferSubmitInfo VulkanInitializers::CommandBufferSubmitInfo(VkCommandBuffer cmd)
	{
		VkCommandBufferSubmitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		info.pNext = nullptr;
		info.commandBuffer = cmd;
		info.deviceMask = 0;

		return info;
	}

	VkSemaphoreSubmitInfo VulkanInitializers::SemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore)
	{
		VkSemaphoreSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.semaphore = semaphore;
		submitInfo.stageMask = stageMask;
		submitInfo.deviceIndex = 0;
		submitInfo.value = 1;

		return submitInfo;
	}

	VkSubmitInfo2 VulkanInitializers::SubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo)
	{
		VkSubmitInfo2 info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		info.pNext = nullptr;

		info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
		info.pWaitSemaphoreInfos = waitSemaphoreInfo;

		info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
		info.pSignalSemaphoreInfos = signalSemaphoreInfo;

		info.commandBufferInfoCount = 1;
		info.pCommandBufferInfos = cmd;

		return info;
	}

	VkImageCreateInfo VulkanInitializers::ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkSampleCountFlagBits samples, VkExtent3D extent)
	{
		VkImageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext = nullptr;

		info.imageType = VK_IMAGE_TYPE_2D;

		info.format = format;
		info.extent = extent;

		info.mipLevels = 1;
		info.arrayLayers = 1;

		//for MSAA
		info.samples = samples;

		//optimal tiling, which means the image is stored on the best gpu format
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = usageFlags;

		return info;
	}

	VkImageViewCreateInfo VulkanInitializers::ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.pNext = nullptr;

		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.image = image;
		info.format = format;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;
		info.subresourceRange.aspectMask = aspectFlags;

		return info;
	}

	VkImageSubresourceRange VulkanInitializers::ImageSubresourceRange(VkImageAspectFlags aspectMask, uint32_t baseMipLevel /*= 0*/)
	{
		VkImageSubresourceRange subImage{};
		subImage.aspectMask = aspectMask;
		subImage.baseMipLevel = baseMipLevel;
		subImage.levelCount = VK_REMAINING_MIP_LEVELS;
		subImage.baseArrayLayer = 0;
		subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

		return subImage;
	}

	VkRenderingAttachmentInfo VulkanInitializers::AttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/)
	{
		VkRenderingAttachmentInfo colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachment.pNext = nullptr;

		colorAttachment.imageView = view;
		colorAttachment.imageLayout = layout;
		colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		if (clear) {
			colorAttachment.clearValue = *clear;
		}

		return colorAttachment;
	}

	VkRenderingInfo VulkanInitializers::RenderingInfo(VkExtent2D extent, const std::vector<VkRenderingAttachmentInfo>& colorAttachments, VkRenderingAttachmentInfo* depthAttachment)
	{
		VkRenderingInfo renderInfo{};
		renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderInfo.pNext = nullptr;

		renderInfo.renderArea = VkRect2D{ VkOffset2D { 0, 0 }, extent };
		renderInfo.layerCount = 1;
		renderInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
		renderInfo.pColorAttachments = colorAttachments.data();
		renderInfo.pDepthAttachment = depthAttachment;	
		renderInfo.pStencilAttachment = nullptr;

		return renderInfo;
	}

	VkRenderingInfo VulkanInitializers::RenderingInfo(VkExtent2D extent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment /*= nullptr*/)
	{
		VkRenderingInfo renderInfo{};
		renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderInfo.pNext = nullptr;

		renderInfo.renderArea = VkRect2D{ VkOffset2D { 0, 0 }, extent };
		renderInfo.layerCount = 1;
		renderInfo.colorAttachmentCount = 1;
		renderInfo.pColorAttachments = colorAttachment;
		renderInfo.pDepthAttachment = depthAttachment;
		renderInfo.pStencilAttachment = depthAttachment;

		return renderInfo;
	}

}