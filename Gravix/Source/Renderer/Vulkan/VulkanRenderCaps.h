#pragma once

#include "VulkanDevice.h"

namespace Gravix
{

	class VulkanRenderCaps
	{
	public:
		static void Init(VulkanDevice* device);

		static VkSampleCountFlagBits GetSampleCount();

		static uint32_t GetRecommendedBindlessSamplers();
		static uint32_t GetRecommendedBindlessSampledImages();
		static uint32_t GetRecommendedBindlessStorageImages();
		static uint32_t GetRecommendedBindlessStorageBuffers();

		static uint32_t GetMaxBoundDescriptorSets();
		static uint32_t GetMaxDescriptorSetUniformBuffers();
	private:
		static void GetSampleCount(VkPhysicalDevice physicalDevice);
		static void GetMaxDescriptorSlots(VkPhysicalDevice physicalDevice);
		static void CalculateBindlessLimits(const VkPhysicalDeviceProperties& properties);
	};

}