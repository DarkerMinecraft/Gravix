#include "pch.h"
#include "VulkanDevice.h"

#include <VkBootstrap.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	// Filter out GOG Galaxy layer warnings
	std::string message = pCallbackData->pMessage;
	if (message.find("GalaxyOverlayVkLayer") != std::string::npos &&
		message.find("Policy #LLP_LAYER_3") != std::string::npos)
	{
		return VK_FALSE; // Suppress this message
	}

	// Map Vulkan severity to your logging system
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		EN_CORE_ERROR("Vulkan Validation: {}", pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		EN_CORE_WARN("Vulkan Validation: {}", pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		EN_CORE_INFO("Vulkan Validation: {}", pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		EN_CORE_TRACE("Vulkan Validation: {}", pCallbackData->pMessage);
		break;
	default:
		EN_CORE_INFO("Vulkan Validation: {}", pCallbackData->pMessage);
		break;
	}

	return VK_FALSE;
}

namespace Gravix 
{
	
	VulkanDevice::VulkanDevice(const DeviceProperties& deviceProperties)
	{
		InitVulkan(deviceProperties);
		CreateSwapchain(deviceProperties.Width, deviceProperties.Height, deviceProperties.VSync);
	}

	VulkanDevice::~VulkanDevice()
	{
		DestroySwapchain();

		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyDevice(m_Device, nullptr);

		if (m_UseValidationLayer)
			vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
		vkDestroyInstance(m_Instance, nullptr);
	}

	void VulkanDevice::InitVulkan(const DeviceProperties& properties)
	{
		vkb::InstanceBuilder builder;

		auto instBuilder = builder.set_app_name("Gravix Engine")
			.set_engine_name("Gravix")
			.set_engine_version(1, 0, 0)
			.set_app_version(1, 0, 0)
			.require_api_version(1, 4, 0);

		if (m_UseValidationLayer)
		{
			instBuilder.request_validation_layers(true);

			// Configure custom debug messenger
			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
			debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugCreateInfo.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugCreateInfo.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugCreateInfo.pfnUserCallback = debugCallback;

			instBuilder.set_debug_callback(&debugCallback);
		}

		auto instRet = instBuilder.build();
		m_Instance = instRet.value();

		if(m_UseValidationLayer)
			m_DebugMessenger = instRet.value().debug_messenger;

#ifdef ENGINE_PLATFORM_WINDOWS
		VkWin32SurfaceCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = (HWND)properties.WindowHandle;
		createInfo.hinstance = GetModuleHandle(NULL);

		VkResult result = vkCreateWin32SurfaceKHR(m_Instance, &createInfo, nullptr, &m_Surface);
		if (result != VK_SUCCESS)
		{
			EN_CORE_CRITICAL("Failed to create Win32 surface: {}", static_cast<int>(result));
			return;
		}
#endif

		//Vulkan 1.4 features 
		VkPhysicalDeviceVulkan14Features features14{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES };

		//Vulkan 1.3 features
		VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		features13.dynamicRendering = true;
		features13.synchronization2 = true;

		//Vulkan 1.2 features
		VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		features12.bufferDeviceAddress = true;
		features12.descriptorIndexing = true;

		VkPhysicalDeviceFeatures features{};

		//We want a gpu that can write to the win32 surface and supports vulkan 1.4 with the correct features
		vkb::PhysicalDeviceSelector selector{ instRet.value() };
		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 4)
			.set_required_features_14(features14)
			.set_required_features_13(features13)
			.set_required_features_12(features12)
			.set_required_features(features)
			.set_surface(m_Surface)
			.select()
			.value();

		//create the final vulkan device
		vkb::DeviceBuilder deviceBuilder{ physicalDevice };

		vkb::Device vkbDevice = deviceBuilder.build().value();

		// Get the VkDevice handle used in the rest of a vulkan application
		m_Device = vkbDevice.device;
		m_PhysicalDevice = physicalDevice.physical_device;
	}

	void VulkanDevice::CreateSwapchain(uint32_t width, uint32_t height, bool vSync)
	{
		vkb::SwapchainBuilder swapchainBuilder{ m_PhysicalDevice, m_Device, m_Surface };

		m_SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

		if(vSync)
			swapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR);
		else
			swapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR);

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.set_desired_format(VkSurfaceFormatKHR{ .format = m_SwapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			.set_desired_extent(width, height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		m_SwapchainExtent = vkbSwapchain.extent;
		//store swapchain and its related images
		m_Swapchain = vkbSwapchain.swapchain;
		m_SwapchainImages = vkbSwapchain.get_images().value();
		m_SwapchainImageViews = vkbSwapchain.get_image_views().value();
	}

	void VulkanDevice::DestroySwapchain()
	{
		vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);

		// destroy swapchain resources
		for (int i = 0; i < m_SwapchainImageViews.size(); i++) {

			vkDestroyImageView(m_Device, m_SwapchainImageViews[i], nullptr);
		}
	}

	void VulkanDevice::RecreateSwapchain(uint32_t width, uint32_t height, bool vSync)
	{

	}


}