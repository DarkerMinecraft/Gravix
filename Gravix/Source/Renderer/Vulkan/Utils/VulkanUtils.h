#pragma once

#include <vulkan/vulkan.h>

namespace Gravix 
{

	class VulkanUtils 
	{
	public:
		static void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
	};

}