#include "pch.h"
#include "VulkanCommandSetup.h"

#include "VulkanInitializers.h"
#include "Renderer/Vulkan/VulkanDevice.h" // For FrameData definition

namespace Gravix
{

	void VulkanCommandSetup::InitializeFrameData(VkDevice device, uint32_t graphicsQueueFamilyIndex, FrameData* frames, uint32_t frameCount)
	{
		VkCommandPoolCreateInfo commandPoolInfo = VulkanInitializers::CommandPoolCreateInfo(
			graphicsQueueFamilyIndex,
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
		);

		VkFenceCreateInfo fenceCreateInfo = VulkanInitializers::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreCreateInfo = VulkanInitializers::SemaphoreCreateInfo();

		for (uint32_t i = 0; i < frameCount; i++)
		{
			// Create command pool
			vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].CommandPool);

			// Allocate command buffer
			VkCommandBufferAllocateInfo cmdAllocInfo = VulkanInitializers::CommandBufferAllocateInfo(
				frames[i].CommandPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1
			);
			vkAllocateCommandBuffers(device, &cmdAllocInfo, &frames[i].CommandBuffer);

			// Create sync structures
			vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].RenderFence);
			vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].SwapchainSemaphore);
		}
	}

	VulkanCommandSetupResult VulkanCommandSetup::InitializeImmediate(VkDevice device, uint32_t graphicsQueueFamilyIndex)
	{
		VulkanCommandSetupResult result{};

		VkCommandPoolCreateInfo commandPoolInfo = VulkanInitializers::CommandPoolCreateInfo(
			graphicsQueueFamilyIndex,
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
		);

		// Create immediate submit command pool and buffer
		vkCreateCommandPool(device, &commandPoolInfo, nullptr, &result.ImmediateCommandPool);

		VkCommandBufferAllocateInfo cmdAllocInfo = VulkanInitializers::CommandBufferAllocateInfo(
			result.ImmediateCommandPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1
		);
		vkAllocateCommandBuffers(device, &cmdAllocInfo, &result.ImmediateCommandBuffer);

		// Create immediate fence
		VkFenceCreateInfo fenceCreateInfo = VulkanInitializers::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		vkCreateFence(device, &fenceCreateInfo, nullptr, &result.ImmediateFence);

		return result;
	}

}
