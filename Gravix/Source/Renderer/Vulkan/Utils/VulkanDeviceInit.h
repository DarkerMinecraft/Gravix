#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Gravix
{
	struct DeviceProperties;

	struct VulkanDeviceInitResult
	{
		VkInstance Instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;
		VkSurfaceKHR Surface = VK_NULL_HANDLE;
		VkDevice Device = VK_NULL_HANDLE;
		VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
		VmaAllocator Allocator = VK_NULL_HANDLE;
		VkQueue GraphicsQueue = VK_NULL_HANDLE;
		uint32_t GraphicsQueueFamilyIndex = 0;
		VkQueue TransferQueue = VK_NULL_HANDLE;
		uint32_t TransferQueueFamilyIndex = 0;
	};

	class VulkanDeviceInit
	{
	public:
		static VulkanDeviceInitResult Initialize(const DeviceProperties& properties, bool useValidationLayers);
	};

}
