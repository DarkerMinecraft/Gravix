#pragma once

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

namespace Gravix 
{

	struct AllocatedImage
	{
		VkImage Image;
		VkImageView ImageView;
		VmaAllocation Allocation;
		VkExtent3D ImageExtent;
		VkFormat ImageFormat;
		VkImageLayout ImageLayout;
	};

	struct AllocatedBuffer 
	{
		VkBuffer Buffer;
		VmaAllocation Allocation;
		VmaAllocationInfo Info;
	};

}