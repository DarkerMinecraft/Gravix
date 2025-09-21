#pragma once

#include "Renderer/Generic/Device.h"

#ifdef ENGINE_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

namespace Gravix 
{

	class VulkanDevice : public Device
	{
	public:
		VulkanDevice(const DeviceProperties& deviceProperties);
		virtual ~VulkanDevice();
	private:
		void InitVulkan(const DeviceProperties& properties);
		void CreateSwapchain(uint32_t width, uint32_t height, bool vSync);
		void DestroySwapchain();
		void RecreateSwapchain(uint32_t width, uint32_t height, bool vSync);
	private:
		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_Device;
		VkSurfaceKHR m_Surface;

		VkQueue m_GraphicsQueue;
		uint32_t m_GraphicsQueueFamilyIndex;

		VkSwapchainKHR m_Swapchain;
		VkFormat m_SwapchainImageFormat;

		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;
		VkExtent2D m_SwapchainExtent;

#ifdef ENGINE_DEBUG
		bool m_UseValidationLayer = true;
#else 
		bool m_UseValidationLayer = false;
#endif
	};

}