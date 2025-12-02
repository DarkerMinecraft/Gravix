#include "pch.h"
#include "VulkanRenderCaps.h"

#include "Core/Log.h"

namespace Gravix
{

	struct RenderCaps
	{
		VkSampleCountFlagBits MaxSampleCount;
		
		// Descriptor Set Limits (most important for bindless)
		uint32_t MaxDescriptorSetSamplers;
		uint32_t MaxDescriptorSetSampledImages;
		uint32_t MaxDescriptorSetStorageImages;
		uint32_t MaxDescriptorSetStorageBuffers;
		uint32_t MaxDescriptorSetUniformBuffers;
		uint32_t MaxBoundDescriptorSets;

		// Per-Stage Limits
		uint32_t MaxPerStageDescriptorSamplers;
		uint32_t MaxPerStageDescriptorSampledImages;
		uint32_t MaxPerStageDescriptorStorageImages;
		uint32_t MaxPerStageDescriptorStorageBuffers;
		uint32_t MaxPerStageDescriptorUniformBuffers;
		uint32_t MaxPerStageResources;

		// Dynamic descriptor limits
		uint32_t MaxDescriptorSetUniformBuffersDynamic;
		uint32_t MaxDescriptorSetStorageBuffersDynamic;

		// Practical bindless limits (computed)
		uint32_t RecommendedBindlessSamplers;
		uint32_t RecommendedBindlessSampledImages;
		uint32_t RecommendedBindlessStorageImages;
		uint32_t RecommendedBindlessStorageBuffers;
	};

	static RenderCaps s_RenderCaps;

	void VulkanRenderCaps::Init(VulkanDevice* device)
	{
		VkPhysicalDevice physicalDevice = device->GetPhysicalDevice();
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

		GX_CORE_INFO("Driver Information:");
		GX_CORE_INFO("     Vendor ID: {0}", physicalDeviceProperties.vendorID);
		GX_CORE_INFO("     Device ID: {0}", physicalDeviceProperties.deviceID);
		GX_CORE_INFO("     Driver Version: {0}", physicalDeviceProperties.driverVersion);
		GX_CORE_INFO("     API Version: {0}", physicalDeviceProperties.apiVersion);
		GX_CORE_INFO("     Device Name: {0}", physicalDeviceProperties.deviceName);

		GX_CORE_INFO("Device Abilities:");
		GetSampleCount(physicalDevice);
		GetMaxDescriptorSlots(physicalDevice);
	}

	void VulkanRenderCaps::GetSampleCount(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

		VkSampleCountFlags counts =
			physicalDeviceProperties.limits.framebufferColorSampleCounts &
			physicalDeviceProperties.limits.framebufferDepthSampleCounts;

		if (counts & VK_SAMPLE_COUNT_64_BIT)
		{
			GX_CORE_INFO("     64x MSAA Supported!");
			s_RenderCaps.MaxSampleCount = VK_SAMPLE_COUNT_64_BIT;
			return;
		}

		if (counts & VK_SAMPLE_COUNT_32_BIT)
		{
			GX_CORE_INFO("     32x MSAA Supported!");
			s_RenderCaps.MaxSampleCount = VK_SAMPLE_COUNT_32_BIT;
			return;
		}

		if (counts & VK_SAMPLE_COUNT_16_BIT)
		{
			GX_CORE_INFO("     16x MSAA Supported!");
			s_RenderCaps.MaxSampleCount = VK_SAMPLE_COUNT_16_BIT;
			return;
		}

		if (counts & VK_SAMPLE_COUNT_8_BIT)
		{
			GX_CORE_INFO("     8x MSAA Supported!");
			s_RenderCaps.MaxSampleCount = VK_SAMPLE_COUNT_8_BIT;
			return;
		}

		if (counts & VK_SAMPLE_COUNT_4_BIT)
		{
			GX_CORE_INFO("     4x MSAA Supported!");
			s_RenderCaps.MaxSampleCount = VK_SAMPLE_COUNT_4_BIT;
			return;
		}

		if (counts & VK_SAMPLE_COUNT_2_BIT)
		{
			GX_CORE_INFO("     2x MSAA Supported!");
			s_RenderCaps.MaxSampleCount = VK_SAMPLE_COUNT_2_BIT;
			return;
		}

		GX_CORE_INFO("     MSAA Not Supported!");
		s_RenderCaps.MaxSampleCount = VK_SAMPLE_COUNT_1_BIT;
	}

	void VulkanRenderCaps::GetMaxDescriptorSlots(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		const VkPhysicalDeviceLimits& limits = properties.limits;

		GX_CORE_INFO("Descriptor Set Limits:");

		// Query all descriptor set limits
		s_RenderCaps.MaxDescriptorSetSamplers = limits.maxDescriptorSetSamplers;
		s_RenderCaps.MaxDescriptorSetSampledImages = limits.maxDescriptorSetSampledImages;
		s_RenderCaps.MaxDescriptorSetStorageImages = limits.maxDescriptorSetStorageImages;
		s_RenderCaps.MaxDescriptorSetStorageBuffers = limits.maxDescriptorSetStorageBuffers;
		s_RenderCaps.MaxDescriptorSetUniformBuffers = limits.maxDescriptorSetUniformBuffers;
		s_RenderCaps.MaxBoundDescriptorSets = limits.maxBoundDescriptorSets;

		GX_CORE_INFO("     Max Descriptor Set Samplers: {0}", s_RenderCaps.MaxDescriptorSetSamplers);
		GX_CORE_INFO("     Max Descriptor Set Sampled Images: {0}", s_RenderCaps.MaxDescriptorSetSampledImages);
		GX_CORE_INFO("     Max Descriptor Set Storage Images: {0}", s_RenderCaps.MaxDescriptorSetStorageImages);
		GX_CORE_INFO("     Max Descriptor Set Storage Buffers: {0}", s_RenderCaps.MaxDescriptorSetStorageBuffers);
		GX_CORE_INFO("     Max Descriptor Set Uniform Buffers: {0}", s_RenderCaps.MaxDescriptorSetUniformBuffers);
		GX_CORE_INFO("     Max Bound Descriptor Sets: {0}", s_RenderCaps.MaxBoundDescriptorSets);

		// Query per-stage limits
		s_RenderCaps.MaxPerStageDescriptorSamplers = limits.maxPerStageDescriptorSamplers;
		s_RenderCaps.MaxPerStageDescriptorSampledImages = limits.maxPerStageDescriptorSampledImages;
		s_RenderCaps.MaxPerStageDescriptorStorageImages = limits.maxPerStageDescriptorStorageImages;
		s_RenderCaps.MaxPerStageDescriptorStorageBuffers = limits.maxPerStageDescriptorStorageBuffers;
		s_RenderCaps.MaxPerStageDescriptorUniformBuffers = limits.maxPerStageDescriptorUniformBuffers;
		s_RenderCaps.MaxPerStageResources = limits.maxPerStageResources;

		GX_CORE_INFO("Per-Stage Limits:");
		GX_CORE_INFO("     Max Per Stage Samplers: {0}", s_RenderCaps.MaxPerStageDescriptorSamplers);
		GX_CORE_INFO("     Max Per Stage Sampled Images: {0}", s_RenderCaps.MaxPerStageDescriptorSampledImages);
		GX_CORE_INFO("     Max Per Stage Storage Images: {0}", s_RenderCaps.MaxPerStageDescriptorStorageImages);
		GX_CORE_INFO("     Max Per Stage Storage Buffers: {0}", s_RenderCaps.MaxPerStageDescriptorStorageBuffers);
		GX_CORE_INFO("     Max Per Stage Uniform Buffers: {0}", s_RenderCaps.MaxPerStageDescriptorUniformBuffers);
		GX_CORE_INFO("     Max Per Stage Resources: {0}", s_RenderCaps.MaxPerStageResources);

		// Query dynamic limits
		s_RenderCaps.MaxDescriptorSetUniformBuffersDynamic = limits.maxDescriptorSetUniformBuffersDynamic;
		s_RenderCaps.MaxDescriptorSetStorageBuffersDynamic = limits.maxDescriptorSetStorageBuffersDynamic;

		GX_CORE_INFO("Dynamic Descriptor Limits:");
		GX_CORE_INFO("     Max Dynamic Uniform Buffers: {0}", s_RenderCaps.MaxDescriptorSetUniformBuffersDynamic);
		GX_CORE_INFO("     Max Dynamic Storage Buffers: {0}", s_RenderCaps.MaxDescriptorSetStorageBuffersDynamic);

		// Calculate practical bindless limits
		CalculateBindlessLimits(properties);
	}

	void VulkanRenderCaps::CalculateBindlessLimits(const VkPhysicalDeviceProperties& properties)
	{
		// Apply practical caps based on hardware characteristics
		uint32_t conservativeMultiplier;
		uint32_t baseBindlessSize;

		// NOTE: These values determine initial descriptor pool sizes and memory usage
		// Reduced from previous values (NVIDIA: 100k -> 1k) to minimize memory footprint
		// Can be increased if you need more simultaneous resources loaded
		// Previous values: NVIDIA=100000, AMD=65536, Intel=32768

		// Determine base bindless size based on GPU vendor/type
		if (properties.vendorID == 0x10DE) // NVIDIA
		{
			baseBindlessSize = 1000; // Start with 1000 descriptors (was 100000)
			conservativeMultiplier = 90; // 90% of max
		}
		else if (properties.vendorID == 0x1002) // AMD
		{
			baseBindlessSize = 1000; // Start with 1000 descriptors (was 65536)
			conservativeMultiplier = 85; // 85% of max
		}
		else if (properties.vendorID == 0x8086) // Intel
		{
			baseBindlessSize = 1000; // Start with 1000 descriptors (was 32768)
			conservativeMultiplier = 80; // 80% of max
		}
		else
		{
			baseBindlessSize = 1000; // Start with 1000 descriptors (was 32768)
			conservativeMultiplier = 75; // 75% of max
		}

		// Mobile/integrated GPUs get lower limits
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			baseBindlessSize = std::min(baseBindlessSize, 500u);
			conservativeMultiplier = std::min(conservativeMultiplier, 70u);
		}

		// Calculate recommended bindless limits as percentage of hardware max, capped by practical limits
		auto calculateLimit = [&](uint32_t hardwareMax, uint32_t practicalMax) -> uint32_t {
			uint32_t conservativeMax = (hardwareMax * conservativeMultiplier) / 100;
			return std::min({ conservativeMax, practicalMax, baseBindlessSize });
			};

		s_RenderCaps.RecommendedBindlessSamplers = calculateLimit(
			s_RenderCaps.MaxDescriptorSetSamplers, baseBindlessSize);

		s_RenderCaps.RecommendedBindlessSampledImages = calculateLimit(
			s_RenderCaps.MaxDescriptorSetSampledImages, baseBindlessSize);

		s_RenderCaps.RecommendedBindlessStorageImages = calculateLimit(
			s_RenderCaps.MaxDescriptorSetStorageImages, baseBindlessSize);

		s_RenderCaps.RecommendedBindlessStorageBuffers = calculateLimit(
			s_RenderCaps.MaxDescriptorSetStorageBuffers, baseBindlessSize);

		GX_CORE_INFO("Recommended Bindless Limits:");
		GX_CORE_INFO("     Bindless Samplers: {0} (from hardware max: {1})",
			s_RenderCaps.RecommendedBindlessSamplers, s_RenderCaps.MaxDescriptorSetSamplers);
		GX_CORE_INFO("     Bindless Sampled Images: {0} (from hardware max: {1})",
			s_RenderCaps.RecommendedBindlessSampledImages, s_RenderCaps.MaxDescriptorSetSampledImages);
		GX_CORE_INFO("     Bindless Storage Images: {0} (from hardware max: {1})",
			s_RenderCaps.RecommendedBindlessStorageImages, s_RenderCaps.MaxDescriptorSetStorageImages);
		GX_CORE_INFO("     Bindless Storage Buffers: {0} (from hardware max: {1})",
			s_RenderCaps.RecommendedBindlessStorageBuffers, s_RenderCaps.MaxDescriptorSetStorageBuffers);

		// Warn if descriptor set limit is low
		if (s_RenderCaps.MaxBoundDescriptorSets <= 4)
		{
			GX_CORE_WARN("     WARNING: Only {0} descriptor sets supported - consider using immutable samplers for bindless!",
				s_RenderCaps.MaxBoundDescriptorSets);
		}
	}

	VkSampleCountFlagBits VulkanRenderCaps::GetSampleCount()
	{
		return s_RenderCaps.MaxSampleCount;
	}

	uint32_t VulkanRenderCaps::GetRecommendedBindlessSamplers()
	{
		return s_RenderCaps.RecommendedBindlessSamplers;
	}

	uint32_t VulkanRenderCaps::GetRecommendedBindlessSampledImages()
	{
		return s_RenderCaps.RecommendedBindlessSampledImages;
	}

	uint32_t VulkanRenderCaps::GetRecommendedBindlessStorageImages()
	{
		return s_RenderCaps.RecommendedBindlessStorageImages;
	}

	uint32_t VulkanRenderCaps::GetRecommendedBindlessStorageBuffers()
	{
		return s_RenderCaps.RecommendedBindlessStorageBuffers;
	}

	uint32_t VulkanRenderCaps::GetMaxBoundDescriptorSets()
	{
		return s_RenderCaps.MaxBoundDescriptorSets;
	}

	uint32_t VulkanRenderCaps::GetMaxDescriptorSetUniformBuffers()
	{
		return s_RenderCaps.MaxDescriptorSetUniformBuffers;
	}

}
