#include "pch.h"
#include "VulkanDevice.h"
#include "VulkanRenderCaps.h"

#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Gravix 
{
	
	VulkanDevice::VulkanDevice(const DeviceProperties& deviceProperties)
	{
		InitVulkan(deviceProperties);
		CreateSwapchain(deviceProperties.Width, deviceProperties.Height, deviceProperties.VSync);
		VulkanRenderCaps::Init(this);
		InitDescriptorPool();
	}

	VulkanDevice::~VulkanDevice()
	{
		vkDeviceWaitIdle(m_Device);

		// Destroy descriptor set layouts (must be done before destroying the pool)
		if (m_BindlessStorageBufferLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(m_Device, m_BindlessStorageBufferLayout, nullptr);

		if (m_BindlessSampledImageLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(m_Device, m_BindlessSampledImageLayout, nullptr);

		if (m_BindlessStorageImageLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(m_Device, m_BindlessStorageImageLayout, nullptr);

		if (m_BindlessSamplerLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(m_Device, m_BindlessSamplerLayout, nullptr);

		// Destroy descriptor pool (this automatically frees all descriptor sets)
		if (m_DescriptorPool != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);

		vmaDestroyAllocator(m_Allocator);

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

		auto instRet = builder.set_app_name("Gravix Engine")
			.set_engine_name("Gravix")
			.set_engine_version(1, 0, 0)
			.set_app_version(1, 0, 0)
			.require_api_version(1, 4, 0)
			.request_validation_layers(m_UseValidationLayer)
			.use_default_debug_messenger()
			.build();
		
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
			GX_CORE_CRITICAL("Failed to create Win32 surface: {}", static_cast<int>(result));
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
		features12.descriptorBindingPartiallyBound = true;
		features12.descriptorBindingVariableDescriptorCount = true;
		features12.descriptorBindingSampledImageUpdateAfterBind = true;
		features12.descriptorBindingStorageBufferUpdateAfterBind = true;
		features12.descriptorBindingStorageImageUpdateAfterBind = true;
		features12.descriptorBindingUniformBufferUpdateAfterBind = true;
		features12.runtimeDescriptorArray = true;

		VkPhysicalDeviceFeatures features{};

		//We want a gpu that can write to the win32 surface and supports vulkan 1.4 with the correct features
		vkb::PhysicalDeviceSelector selector{ instRet.value() };
		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 4)
			.add_required_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
			.add_required_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
			.add_required_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
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

		// Create VMA Allocator
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = m_PhysicalDevice;
		allocatorInfo.device = m_Device;
		allocatorInfo.instance = m_Instance;
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
		vmaCreateAllocator(&allocatorInfo, &m_Allocator);
	}

	void VulkanDevice::CreateSwapchain(uint32_t width, uint32_t height, bool vSync)
	{
		vkb::SwapchainBuilder swapchainBuilder{ m_PhysicalDevice, m_Device, m_Surface };

		m_SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.set_desired_format(VkSurfaceFormatKHR{ .format = m_SwapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			.set_desired_extent(width, height)
			.set_desired_present_mode(vSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR)
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
		vkb::SwapchainBuilder swapchainBuilder{ m_PhysicalDevice, m_Device, m_Surface };

		m_SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.set_desired_format(VkSurfaceFormatKHR{ .format = m_SwapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			.set_desired_extent(width, height)
			.set_desired_present_mode(vSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		m_SwapchainExtent = vkbSwapchain.extent;
		//store swapchain and its related images
		m_Swapchain = vkbSwapchain.swapchain;
		m_SwapchainImages = vkbSwapchain.get_images().value();
		m_SwapchainImageViews = vkbSwapchain.get_image_views().value();
	}


	void VulkanDevice::InitDescriptorPool()
	{
		// Get recommended bindless limits from capabilities
		uint32_t maxSamplers = VulkanRenderCaps::GetRecommendedBindlessSamplers();
		uint32_t maxSampledImages = VulkanRenderCaps::GetRecommendedBindlessSampledImages();
		uint32_t maxStorageImages = VulkanRenderCaps::GetRecommendedBindlessStorageImages();
		uint32_t maxStorageBuffers = VulkanRenderCaps::GetRecommendedBindlessStorageBuffers();
		uint32_t maxUniformBuffers = VulkanRenderCaps::GetMaxDescriptorSetUniformBuffers();

		// Apply reasonable limits for uniform buffers (not typically bindless)
		maxUniformBuffers = std::min(maxUniformBuffers, 1000u);

		GX_CORE_INFO("Creating Descriptor Pool with:");
		GX_CORE_INFO("     Samplers: {0}", maxSamplers);
		GX_CORE_INFO("     Sampled Images: {0}", maxSampledImages);
		GX_CORE_INFO("     Storage Images: {0}", maxStorageImages);
		GX_CORE_INFO("     Storage Buffers: {0}", maxStorageBuffers);
		GX_CORE_INFO("     Uniform Buffers: {0}", maxUniformBuffers);

		// Define pool sizes for each descriptor type
		std::vector<VkDescriptorPoolSize> poolSizes = {
			// Bindless resource types
			{ VK_DESCRIPTOR_TYPE_SAMPLER, maxSamplers },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxSampledImages },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxSampledImages }, // Alternative binding method
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxStorageImages },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxStorageBuffers },

			// Regular descriptor types (for non-bindless descriptors)
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxUniformBuffers },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },  // For dynamic uniforms
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },  // For dynamic storage

			// Ray tracing descriptors (if supported)
			{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1000 }
		};

		// Calculate total descriptor sets needed
		// We need at least 4 bindless sets + some extra for regular descriptor sets
		uint32_t maxBoundSets = VulkanRenderCaps::GetMaxBoundDescriptorSets();
		uint32_t maxDescriptorSets = std::max(maxBoundSets * 10, 1000u); // 10x safety margin for regular sets

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT |
			VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT; // Required for bindless
		poolInfo.maxSets = maxDescriptorSets;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();

		VkResult result = vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool);
		if (result != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to create descriptor pool! Error: {0}", static_cast<int>(result));
			throw std::runtime_error("Failed to create descriptor pool");
		}

		GX_CORE_INFO("Descriptor Pool created successfully with {0} max sets", maxDescriptorSets);

		// Optionally create bindless descriptor set layouts and sets here
		CreateBindlessDescriptorSets();
	}

	void VulkanDevice::CreateBindlessDescriptorSets()
	{
		uint32_t maxSamplers = VulkanRenderCaps::GetRecommendedBindlessSamplers();
		uint32_t maxSampledImages = VulkanRenderCaps::GetRecommendedBindlessSampledImages();
		uint32_t maxStorageImages = VulkanRenderCaps::GetRecommendedBindlessStorageImages();
		uint32_t maxStorageBuffers = VulkanRenderCaps::GetRecommendedBindlessStorageBuffers();

		// Create layouts as before...
		CreateBindlessLayout(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxStorageBuffers,
			VK_SHADER_STAGE_ALL, &m_BindlessStorageBufferLayout);
		CreateBindlessLayout(1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxSampledImages,
			VK_SHADER_STAGE_ALL, &m_BindlessSampledImageLayout);
		CreateBindlessLayout(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxStorageImages,
			VK_SHADER_STAGE_ALL, &m_BindlessStorageImageLayout);
		CreateBindlessLayout(3, VK_DESCRIPTOR_TYPE_SAMPLER, maxSamplers,
			VK_SHADER_STAGE_ALL, &m_BindlessSamplerLayout);

		VkDescriptorSetLayout layouts[] = {
			m_BindlessStorageBufferLayout,
			m_BindlessSampledImageLayout,
			m_BindlessStorageImageLayout,
			m_BindlessSamplerLayout
		};

		// Specify actual descriptor counts for variable bindings
		uint32_t variableCounts[] = { maxStorageBuffers, maxSampledImages, maxStorageImages, maxSamplers };

		VkDescriptorSetVariableDescriptorCountAllocateInfo variableInfo = {};
		variableInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
		variableInfo.descriptorSetCount = 4;
		variableInfo.pDescriptorCounts = variableCounts;

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = &variableInfo;  // Add this!
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 4;
		allocInfo.pSetLayouts = layouts;

		VkResult result = vkAllocateDescriptorSets(m_Device, &allocInfo, m_BindlessDescriptorSets);
		if (result != VK_SUCCESS) {
			GX_CORE_ERROR("Failed to allocate bindless descriptor sets! Error: {0}", static_cast<int>(result));
			throw std::runtime_error("Failed to allocate bindless descriptor sets");
		}
	}

	void VulkanDevice::CreateBindlessLayout(uint32_t binding, VkDescriptorType type, uint32_t count, VkShaderStageFlags stages, VkDescriptorSetLayout* layout)
	{
		VkDescriptorSetLayoutBinding bindlessBinding = {};
		bindlessBinding.binding = binding;
		bindlessBinding.descriptorType = type;
		bindlessBinding.descriptorCount = count;
		bindlessBinding.stageFlags = stages;
		bindlessBinding.pImmutableSamplers = nullptr;

		// Enable bindless features
		VkDescriptorBindingFlags bindingFlags =
			VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

		VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {};
		bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		bindingFlagsInfo.bindingCount = 1;
		bindingFlagsInfo.pBindingFlags = &bindingFlags;

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.pNext = &bindingFlagsInfo;
		layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &bindlessBinding;

		VkResult result = vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, layout);
		if (result != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to create bindless descriptor set layout! Error: {0}", static_cast<int>(result));
			throw std::runtime_error("Failed to create bindless descriptor set layout");
		}
	}
}