#pragma once

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

namespace Gravix 
{

	struct AllocatedImage 
	{
		VkImage Image;
		VmaAllocation Allocation;
		VkImageView ImageView;
		VkFormat Format;
	};

}