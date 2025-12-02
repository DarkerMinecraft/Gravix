#pragma once

#include <vulkan/vulkan.h>

namespace Gravix
{

	struct FrameData; // Forward declaration

	struct VulkanCommandSetupResult
	{
		VkCommandPool ImmediateCommandPool = VK_NULL_HANDLE;
		VkCommandBuffer ImmediateCommandBuffer = VK_NULL_HANDLE;
		VkFence ImmediateFence = VK_NULL_HANDLE;
	};

	class VulkanCommandSetup
	{
	public:
		// Initialize frame data (command pools, buffers, sync structures)
		static void InitializeFrameData(VkDevice device, uint32_t graphicsQueueFamilyIndex, FrameData* frames, uint32_t frameCount);

		// Initialize immediate submit structures
		static VulkanCommandSetupResult InitializeImmediate(VkDevice device, uint32_t graphicsQueueFamilyIndex);
	};

}
