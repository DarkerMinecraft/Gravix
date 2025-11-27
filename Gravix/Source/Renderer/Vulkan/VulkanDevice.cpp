#include "pch.h"
#include "VulkanDevice.h"
#include "VulkanRenderCaps.h"

#include "Utils/VulkanInitializers.h"
#include "Core/Application.h"

#include "Utils/VulkanUtils.h"
#include "Debug/Instrumentor.h"

#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Gravix 
{
	
	VulkanDevice::VulkanDevice(const DeviceProperties& deviceProperties)
		: m_Vsync(deviceProperties.VSync)
	{
		InitVulkan(deviceProperties);

		// Create and initialize swapchain
		m_Swapchain = CreateScope<VulkanSwapchain>(m_Device, m_PhysicalDevice, m_Surface);
		m_Swapchain->Create(deviceProperties.Width, deviceProperties.Height, deviceProperties.VSync);

		VulkanRenderCaps::Init(this);
		InitCommandBuffers();
		InitSyncStructures();
		InitDescriptorPool();
		CreateImGuiPool();

		m_ShaderCompiler = CreateRef<ShaderCompiler>();
	}

	VulkanDevice::~VulkanDevice()
	{
		m_ShaderCompiler = nullptr; // Automatically cleaned up by Ref<>

		// Wait for all operations to complete before cleanup
		if (m_Device != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(m_Device);
		}

		m_Swapchain.reset();

		// Clean up per-frame resources
		for (uint32_t i = 0; i < FRAME_OVERLAP; i++)
		{
			if (m_Frames[i].CommandPool != VK_NULL_HANDLE)
			{
				vkDestroyCommandPool(m_Device, m_Frames[i].CommandPool, nullptr);
				m_Frames[i].CommandPool = VK_NULL_HANDLE;
			}

			if (m_Frames[i].SwapchainSemaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(m_Device, m_Frames[i].SwapchainSemaphore, nullptr);
				m_Frames[i].SwapchainSemaphore = VK_NULL_HANDLE;
			}

			if (m_Frames[i].RenderFence != VK_NULL_HANDLE)
			{
				vkDestroyFence(m_Device, m_Frames[i].RenderFence, nullptr);
				m_Frames[i].RenderFence = VK_NULL_HANDLE;
			}
		}

		vkDestroyCommandPool(m_Device, m_ImmediateCommandPool, nullptr);
		vkDestroyFence(m_Device, m_ImmediateFence, nullptr);

		// Destroy bindless descriptor set layouts (must be done before destroying the pool)
		if (m_BindlessStorageBufferLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(m_Device, m_BindlessStorageBufferLayout, nullptr);
			m_BindlessStorageBufferLayout = VK_NULL_HANDLE;
		}

		if (m_BindlessCombinedImageSamplerLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(m_Device, m_BindlessCombinedImageSamplerLayout, nullptr);
			m_BindlessCombinedImageSamplerLayout = VK_NULL_HANDLE;
		}

		if (m_BindlessStorageImageLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(m_Device, m_BindlessStorageImageLayout, nullptr);
			m_BindlessStorageImageLayout = VK_NULL_HANDLE;
		}

		m_BindlessSetLayouts.clear();

		// Destroy descriptor pool (this automatically frees all descriptor sets)
		if (m_DescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
			m_DescriptorPool = VK_NULL_HANDLE;
		}

		if (m_ImGuiDescriptorPool != VK_NULL_HANDLE) 
		{
			vkDestroyDescriptorPool(m_Device, m_ImGuiDescriptorPool, nullptr);
			m_ImGuiDescriptorPool = VK_NULL_HANDLE;
		}

		// Destroy VMA allocator before destroying the device
		if (m_Allocator != VK_NULL_HANDLE)
		{
			vmaDestroyAllocator(m_Allocator);
			m_Allocator = VK_NULL_HANDLE;
		}

		// Swapchain cleanup handled by VulkanSwapchain destructor

		// Destroy surface
		if (m_Surface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
			m_Surface = VK_NULL_HANDLE;
		}

		// Destroy device
		if (m_Device != VK_NULL_HANDLE)
		{
			vkDestroyDevice(m_Device, nullptr);
			m_Device = VK_NULL_HANDLE;
		}

		// Destroy debug messenger
		if (m_UseValidationLayer && m_DebugMessenger != VK_NULL_HANDLE)
		{
			vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
			m_DebugMessenger = VK_NULL_HANDLE;
		}

		// Destroy instance
		if (m_Instance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(m_Instance, nullptr);
			m_Instance = VK_NULL_HANDLE;
		}
	}

	void VulkanDevice::StartFrame()
	{
		GX_PROFILE_FUNCTION();

		m_FrameStarted = false;

		// Always wait for the fence from FRAME_OVERLAP frames ago
		{
			GX_PROFILE_SCOPE("WaitForFences");
			vkWaitForFences(m_Device, 1, &GetCurrentFrameData().RenderFence, true, UINT64_MAX);
		}

		// Skip rendering if window is minimized (zero dimensions)
		uint32_t width = Application::Get().GetWindow().GetWidth();
		uint32_t height = Application::Get().GetWindow().GetHeight();
		if (width == 0 || height == 0)
		{
			// Re-signal the fence immediately so it can be reused next frame
			vkResetFences(m_Device, 1, &GetCurrentFrameData().RenderFence);
			// Submit an empty command buffer to signal the fence
			VkSubmitInfo2 emptySubmit = VulkanInitializers::SubmitInfo(nullptr, nullptr, nullptr);
			vkQueueSubmit2(m_GraphicsQueue, 1, &emptySubmit, GetCurrentFrameData().RenderFence);
			return;
		}

		// Reset fence for this frame
		vkResetFences(m_Device, 1, &GetCurrentFrameData().RenderFence);

		{
			GX_PROFILE_SCOPE("AcquireSwapchainImage");
			// Acquire next swapchain image - signal per-frame semaphore when ready
			uint32_t imageIndex;
			VkResult result = m_Swapchain->AcquireNextImage(GetCurrentFrameData().SwapchainSemaphore, &imageIndex);
			if (result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				m_Swapchain->Recreate(width, height, m_Vsync);
				return;
			}
			else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			{
				GX_CORE_CRITICAL("Failed to acquire swapchain image!");
				return;
			}
		}

		{
			GX_PROFILE_SCOPE("BeginCommandBuffer");
			//begin the command buffer recording
			vkResetCommandBuffer(GetCurrentFrameData().CommandBuffer, 0);

			VkCommandBufferBeginInfo beginInfo = VulkanInitializers::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			vkBeginCommandBuffer(GetCurrentFrameData().CommandBuffer, &beginInfo);

			m_SwapchainImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			//transition the swapchain image to a color attachment
			VulkanUtils::TransitionImage(GetCurrentFrameData().CommandBuffer, GetCurrentSwapchainImage(),
				GetSwapchainImageFormat(), m_SwapchainImageLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			m_SwapchainImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}

		m_FrameStarted = true;
	}

	void VulkanDevice::EndFrame()
	{
		GX_PROFILE_FUNCTION();

		// Only submit and present if we successfully started the frame
		if (m_FrameStarted)
		{
			uint32_t width = Application::Get().GetWindow().GetWidth();
			uint32_t height = Application::Get().GetWindow().GetHeight();

			{
				GX_PROFILE_SCOPE("TransitionImageForPresent");
				VulkanUtils::TransitionImage(GetCurrentFrameData().CommandBuffer, GetCurrentSwapchainImage(), GetSwapchainImageFormat(),
					m_SwapchainImageLayout, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
				m_SwapchainImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

				//end the command buffer recording
				vkEndCommandBuffer(GetCurrentFrameData().CommandBuffer);
			}

			{
				GX_PROFILE_SCOPE("SubmitCommandBuffer");
				VkCommandBufferSubmitInfo cmdinfo = VulkanInitializers::CommandBufferSubmitInfo(GetCurrentFrameData().CommandBuffer);

				// Wait on per-frame acquire semaphore (signaled by vkAcquireNextImageKHR)
				VkSemaphoreSubmitInfo waitInfo = VulkanInitializers::SemaphoreSubmitInfo(
					VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
					GetCurrentFrameData().SwapchainSemaphore);

				// Signal per-swapchain-image present semaphore (waited on by vkQueuePresentKHR)
				VkSemaphoreSubmitInfo signalInfo = VulkanInitializers::SemaphoreSubmitInfo(
					VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
					m_Swapchain->GetCurrentRenderSemaphore());

				VkSubmitInfo2 submit = VulkanInitializers::SubmitInfo(&cmdinfo, &signalInfo, &waitInfo);

				//submit command buffer to the queue and execute it.
				// _renderFence will now block until the graphic commands finish execution
				vkQueueSubmit2(m_GraphicsQueue, 1, &submit, GetCurrentFrameData().RenderFence);
			}

			{
				GX_PROFILE_SCOPE("PresentSwapchain");
				// Present the swapchain image
				VkResult result = m_Swapchain->Present(m_GraphicsQueue, m_Swapchain->GetCurrentImageIndex());

				if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
				{
					m_Swapchain->Recreate(width, height, m_Vsync);
				}
				else if (result != VK_SUCCESS)
				{
					GX_CORE_CRITICAL("Failed to present swapchain image!");
				}
			}
		}

		//always increase the number of frames drawn, even if we skipped rendering
		m_CurrentFrame++;
	}

	void VulkanDevice::WaitIdle()
	{
		vkDeviceWaitIdle(m_Device);
	}

	AllocatedImage VulkanDevice::CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool useSamples /*= false*/, bool mipmapped /*= false*/)
	{
		AllocatedImage newImage;
		newImage.ImageFormat = format;
		newImage.ImageExtent = size;

		VkImageCreateInfo imgInfo = VulkanInitializers::ImageCreateInfo(format, usage, useSamples ? VulkanRenderCaps::GetSampleCount() : VK_SAMPLE_COUNT_1_BIT, size);
		if (mipmapped) {
			imgInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
		}

		// always allocate images on dedicated GPU memory
		VmaAllocationCreateInfo allocinfo{};
		allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// allocate and create the image
		vmaCreateImage(m_Allocator, &imgInfo, &allocinfo, &newImage.Image, &newImage.Allocation, nullptr);

		// if the format is a depth format, we will need to have it use the correct
		// aspect flag
		VkImageAspectFlags aspectFlag = 0;
		bool isDepth = VulkanUtils::IsDepthFormat(format);
		bool isStencil = VulkanUtils::IsStencilFormat(format);
		if(isDepth)
			aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (isStencil)
			aspectFlag |= VK_IMAGE_ASPECT_STENCIL_BIT;
		if (!isDepth && !isStencil)
			aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;

		// build a image-view for the image
		VkImageViewCreateInfo viewInfo = VulkanInitializers::ImageViewCreateInfo(format, newImage.Image, aspectFlag);
		viewInfo.subresourceRange.levelCount = imgInfo.mipLevels;

		vkCreateImageView(m_Device, &viewInfo, nullptr, &newImage.ImageView);

		newImage.ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		return newImage;
	}

	AllocatedImage VulkanDevice::CreateImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped /*= false*/)
	{
		size_t dataSize = size.depth * size.width * size.height * 4;
		AllocatedBuffer uploadbuffer = CreateBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		memcpy(uploadbuffer.Info.pMappedData, data, dataSize);

		AllocatedImage newImage = CreateImage(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, false, mipmapped);

		ImmediateSubmit([&](VkCommandBuffer cmd) {
			VulkanUtils::TransitionImage(cmd, newImage.Image, newImage.ImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;

			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = size;

			// copy the buffer into the image
			vkCmdCopyBufferToImage(cmd, uploadbuffer.Buffer, newImage.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
				&copyRegion);

			VulkanUtils::TransitionImage(cmd, newImage.Image, newImage.ImageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			});

		DestroyBuffer(uploadbuffer);
		newImage.ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		return newImage;
	}

	AllocatedBuffer VulkanDevice::CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
	{
		VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.pNext = nullptr;
		bufferInfo.size = allocSize;
		bufferInfo.usage = usage;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = memoryUsage;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		AllocatedBuffer newBuffer;

		// allocate the buffer
		vmaCreateBuffer(m_Allocator, &bufferInfo, &vmaallocInfo, &newBuffer.Buffer, &newBuffer.Allocation,
			&newBuffer.Info);

		return newBuffer;
	}

	void VulkanDevice::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
	{
		// Lock mutex to ensure only one thread uses the immediate command buffer at a time
		std::lock_guard<std::mutex> lock(m_ImmediateSubmitMutex);

		// Wait for any previous operations to complete
		VkResult result = vkWaitForFences(m_Device, 1, &m_ImmediateFence, VK_TRUE, UINT64_MAX);
		if (result != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to wait for immediate fence before submit: {}", static_cast<int>(result));
			return;
		}

		// Reset the fence for this submission
		result = vkResetFences(m_Device, 1, &m_ImmediateFence);
		if (result != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to reset immediate fence: {}", static_cast<int>(result));
			return;
		}

		VkCommandBuffer cmd = m_ImmediateCommandBuffer;

		result = vkResetCommandBuffer(cmd, 0);
		if (result != VK_SUCCESS) {
			GX_CORE_ERROR("Failed to reset immediate command buffer: {}", static_cast<int>(result));
			return;
		}

		VkCommandBufferBeginInfo cmdBeginInfo = VulkanInitializers::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		result = vkBeginCommandBuffer(cmd, &cmdBeginInfo);
		if (result != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to begin immediate command buffer: {}", static_cast<int>(result));
			return;
		}

		// Execute the provided function
		function(cmd);

		result = vkEndCommandBuffer(cmd);
		if (result != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to end immediate command buffer: {}", static_cast<int>(result));
			return;
		}

		VkCommandBufferSubmitInfo cmdinfo = VulkanInitializers::CommandBufferSubmitInfo(cmd);
		VkSubmitInfo2 submit = VulkanInitializers::SubmitInfo(&cmdinfo, nullptr, nullptr);

		result = vkQueueSubmit2(m_GraphicsQueue, 1, &submit, m_ImmediateFence);
		if (result != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to submit immediate command buffer: {}", static_cast<int>(result));
			// Fence was reset but submission failed - wait would hang, so just return
			// The mutex will be released and next call will properly wait
			return;
		}

		// Wait for the submission to complete
		result = vkWaitForFences(m_Device, 1, &m_ImmediateFence, VK_TRUE, UINT64_MAX);
		if (result != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to wait for immediate fence after submit: {}", static_cast<int>(result));
		}
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
		features12.scalarBlockLayout = true;  // ADD THIS - fixes struct alignment issue

		//Vulkan 1.1 features (ADD THIS ENTIRE SECTION)
		VkPhysicalDeviceVulkan11Features features11{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
		features11.shaderDrawParameters = true;  // Fixes the DrawParameters capability error

		VkPhysicalDeviceFeatures features{};
		features.samplerAnisotropy = true;  // Fix the sampler warning too
		features.shaderStorageImageMultisample = true;
		features.sampleRateShading = true;
		features.independentBlend = true;
		features.wideLines = true;

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
			.set_required_features_11(features11)
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
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		vmaCreateAllocator(&allocatorInfo, &m_Allocator);

		m_GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		m_GraphicsQueueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		m_TransferQueue = vkbDevice.get_queue(vkb::QueueType::transfer).value();
		m_TransferQueueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();
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
		// Query max bindless counts
		uint32_t maxSamplers = VulkanRenderCaps::GetRecommendedBindlessSamplers();
		uint32_t maxSampledImages = VulkanRenderCaps::GetRecommendedBindlessSampledImages();
		uint32_t maxStorageImages = VulkanRenderCaps::GetRecommendedBindlessStorageImages();
		uint32_t maxStorageBuffers = VulkanRenderCaps::GetRecommendedBindlessStorageBuffers();

		// For combined image samplers, take the smaller of samplers/images (safe bound)
		uint32_t maxCombinedImageSamplers = std::min(maxSamplers, maxSampledImages);

		// Create bindless layouts: each set holds 1 type with max descriptors
		CreateBindlessLayout(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxStorageBuffers, VK_SHADER_STAGE_ALL, &m_BindlessStorageBufferLayout);
		CreateBindlessLayout(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxCombinedImageSamplers, VK_SHADER_STAGE_ALL, &m_BindlessCombinedImageSamplerLayout);
		CreateBindlessLayout(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxStorageImages, VK_SHADER_STAGE_ALL, &m_BindlessStorageImageLayout);

		m_BindlessSetLayouts = {
			m_BindlessStorageBufferLayout,       // set 0
			m_BindlessCombinedImageSamplerLayout,// set 1
			m_BindlessStorageImageLayout         // set 2
		};

		// Variable counts for allocation
		uint32_t variableCounts[] = { maxStorageBuffers, maxCombinedImageSamplers, maxStorageImages };

		VkDescriptorSetVariableDescriptorCountAllocateInfo variableInfo{};
		variableInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
		variableInfo.descriptorSetCount = 3;
		variableInfo.pDescriptorCounts = variableCounts;

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = &variableInfo;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 3;
		allocInfo.pSetLayouts = m_BindlessSetLayouts.data();

		if (vkAllocateDescriptorSets(m_Device, &allocInfo, m_BindlessDescriptorSets) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate bindless descriptor sets!");
		}

		GX_CORE_INFO("Bindless descriptor sets created with max bindings:");
		GX_CORE_INFO("   Storage Buffers:        {0}", maxStorageBuffers);
		GX_CORE_INFO("   Combined Image Samplers:{0}", maxCombinedImageSamplers);
		GX_CORE_INFO("   Storage Images:         {0}", maxStorageImages);
	}


	void VulkanDevice::CreateBindlessLayout(VkDescriptorType type, uint32_t count, VkShaderStageFlags stages, VkDescriptorSetLayout* layout)
	{
		VkDescriptorSetLayoutBinding bindlessBinding = {};
		bindlessBinding.binding = 0; // ALWAYS 0 inside the set
		bindlessBinding.descriptorType = type;
		bindlessBinding.descriptorCount = count;
		bindlessBinding.stageFlags = stages;
		bindlessBinding.pImmutableSamplers = nullptr;

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

		if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, layout) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create bindless descriptor set layout!");
		}
	}
	
	void VulkanDevice::CreateImGuiPool()
	{
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;  // ImGui creates one set per texture
		pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
		pool_info.pPoolSizes = pool_sizes;

		VkResult result = vkCreateDescriptorPool(m_Device, &pool_info, nullptr, &m_ImGuiDescriptorPool);
		if (result != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to create ImGui descriptor pool! Error: {0}", static_cast<int>(result));
			throw std::runtime_error("Failed to create ImGui descriptor pool");
		}

		GX_CORE_INFO("ImGui Descriptor Pool created successfully");
	}

	void VulkanDevice::InitCommandBuffers()
	{
		VkCommandPoolCreateInfo commandPoolInfo = VulkanInitializers::CommandPoolCreateInfo(m_GraphicsQueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (uint32_t i = 0; i < FRAME_OVERLAP; i++) 
		{
			vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_Frames[i].CommandPool);

			// allocate the default command buffer that we will use for rendering
			VkCommandBufferAllocateInfo cmdAllocInfo = VulkanInitializers::CommandBufferAllocateInfo(m_Frames[i].CommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
			vkAllocateCommandBuffers(m_Device, &cmdAllocInfo, &m_Frames[i].CommandBuffer);
		}

		vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_ImmediateCommandPool);

		VkCommandBufferAllocateInfo cmdAllocInfo = VulkanInitializers::CommandBufferAllocateInfo(m_ImmediateCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
		vkAllocateCommandBuffers(m_Device, &cmdAllocInfo, &m_ImmediateCommandBuffer);
	}

	void VulkanDevice::InitSyncStructures()
	{
		VkFenceCreateInfo fenceCreateInfo = VulkanInitializers::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreCreateInfo = VulkanInitializers::SemaphoreCreateInfo();

		// Create per-frame fences and acquire semaphores
		for (uint32_t i = 0; i < FRAME_OVERLAP; i++)
		{
			vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_Frames[i].RenderFence);
			vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].SwapchainSemaphore);
		}

		// Per-swapchain-image present semaphores are now created and managed by VulkanSwapchain

		vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_ImmediateFence);
	}

}