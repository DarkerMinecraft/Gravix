#include "pch.h"
#include "VulkanDeviceInit.h"

#include "Core/Log.h"
#include "Renderer/Generic/Device.h"

#include <VkBootstrap.h>

namespace Gravix
{

	VulkanDeviceInitResult VulkanDeviceInit::Initialize(const DeviceProperties& properties, bool useValidationLayers)
	{
		VulkanDeviceInitResult result{};

		// Create Vulkan instance
		vkb::InstanceBuilder builder;

		auto instRet = builder.set_app_name("Gravix Engine")
			.set_engine_name("Gravix")
			.set_engine_version(1, 0, 0)
			.set_app_version(1, 0, 0)
			.require_api_version(1, 4, 0)
			.request_validation_layers(useValidationLayers)
			.use_default_debug_messenger()
			.build();

		result.Instance = instRet.value();

		if (useValidationLayers)
			result.DebugMessenger = instRet.value().debug_messenger;

		// Create surface
#ifdef ENGINE_PLATFORM_WINDOWS
		VkWin32SurfaceCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = (HWND)properties.WindowHandle;
		createInfo.hinstance = GetModuleHandle(NULL);

		VkResult vkResult = vkCreateWin32SurfaceKHR(result.Instance, &createInfo, nullptr, &result.Surface);
		if (vkResult != VK_SUCCESS)
		{
			GX_CORE_CRITICAL("Failed to create Win32 surface: {}", static_cast<int>(vkResult));
			return result;
		}
#endif

		// Configure Vulkan features
		VkPhysicalDeviceVulkan14Features features14{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES };

		VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		features13.dynamicRendering = true;
		features13.synchronization2 = true;

		VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		features12.bufferDeviceAddress = true;
		features12.descriptorIndexing = true;
		features12.descriptorBindingPartiallyBound = true;
		features12.descriptorBindingVariableDescriptorCount = true;
		features12.descriptorBindingSampledImageUpdateAfterBind = true;
		features12.descriptorBindingStorageBufferUpdateAfterBind = true;
		features12.descriptorBindingStorageImageUpdateAfterBind = true;
		features12.descriptorBindingUniformBufferUpdateAfterBind = true;
		features12.runtimeDescriptorArray = true;
		features12.scalarBlockLayout = true;

		VkPhysicalDeviceVulkan11Features features11{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
		features11.shaderDrawParameters = true;

		VkPhysicalDeviceFeatures features{};
		features.samplerAnisotropy = true;
		features.shaderStorageImageMultisample = true;
		features.sampleRateShading = true;
		features.independentBlend = true;
		features.wideLines = true;

		// Select physical device
		vkb::PhysicalDeviceSelector selector{ instRet.value() };
		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 4)
			.add_required_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
			.add_required_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
			.add_required_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
			.set_required_features_14(features14)
			.set_required_features_13(features13)
			.set_required_features_12(features12)
			.set_required_features_11(features11)
			.set_required_features(features)
			.set_surface(result.Surface)
			.select()
			.value();

		// Create logical device
		vkb::DeviceBuilder deviceBuilder{ physicalDevice };
		vkb::Device vkbDevice = deviceBuilder.build().value();

		result.Device = vkbDevice.device;
		result.PhysicalDevice = physicalDevice.physical_device;

		// Create VMA Allocator
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = result.PhysicalDevice;
		allocatorInfo.device = result.Device;
		allocatorInfo.instance = result.Instance;
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		vmaCreateAllocator(&allocatorInfo, &result.Allocator);

		// Get queues
		result.GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		result.GraphicsQueueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		result.TransferQueue = vkbDevice.get_queue(vkb::QueueType::transfer).value();
		result.TransferQueueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();

		return result;
	}

}
