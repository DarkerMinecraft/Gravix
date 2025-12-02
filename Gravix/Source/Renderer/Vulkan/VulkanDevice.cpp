#include "pch.h"
#include "VulkanDevice.h"
#include "VulkanRenderCaps.h"

#include "Utils/VulkanDeviceInit.h"
#include "Utils/VulkanDescriptorSetup.h"
#include "Utils/VulkanCommandSetup.h"
#include "Utils/VulkanInitializers.h"
#include "Utils/VulkanUtils.h"
#include "Core/Application.h"
#include "Debug/Instrumentor.h"

#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Gravix
{

	VulkanDevice::VulkanDevice(const DeviceProperties& deviceProperties)
		: m_Vsync(deviceProperties.VSync)
	{
		// Initialize Vulkan device
		auto deviceInit = VulkanDeviceInit::Initialize(deviceProperties, m_UseValidationLayer);
		m_Instance = deviceInit.Instance;
		m_DebugMessenger = deviceInit.DebugMessenger;
		m_Surface = deviceInit.Surface;
		m_Device = deviceInit.Device;
		m_PhysicalDevice = deviceInit.PhysicalDevice;
		m_Allocator = deviceInit.Allocator;
		m_GraphicsQueue = deviceInit.GraphicsQueue;
		m_GraphicsQueueFamilyIndex = deviceInit.GraphicsQueueFamilyIndex;
		m_TransferQueue = deviceInit.TransferQueue;
		m_TransferQueueFamilyIndex = deviceInit.TransferQueueFamilyIndex;

		// Create and initialize swapchain
		m_Swapchain = CreateScope<VulkanSwapchain>(m_Device, m_PhysicalDevice, m_Surface);
		m_Swapchain->Create(deviceProperties.Width, deviceProperties.Height, deviceProperties.VSync);

		VulkanRenderCaps::Init(this);

		// Initialize command buffers and sync structures
		VulkanCommandSetup::InitializeFrameData(m_Device, m_GraphicsQueueFamilyIndex, m_Frames, FRAME_OVERLAP);
		auto immediateSetup = VulkanCommandSetup::InitializeImmediate(m_Device, m_GraphicsQueueFamilyIndex);
		m_ImmediateCommandPool = immediateSetup.ImmediateCommandPool;
		m_ImmediateCommandBuffer = immediateSetup.ImmediateCommandBuffer;
		m_ImmediateFence = immediateSetup.ImmediateFence;

		// Initialize descriptor pools and bindless sets
		auto descriptorSetup = VulkanDescriptorSetup::Initialize(m_Device);
		m_DescriptorPool = descriptorSetup.DescriptorPool;
		m_ImGuiDescriptorPool = descriptorSetup.ImGuiDescriptorPool;
		std::copy(std::begin(descriptorSetup.BindlessDescriptorSets), std::end(descriptorSetup.BindlessDescriptorSets), std::begin(m_BindlessDescriptorSets));
		m_BindlessStorageBufferLayout = descriptorSetup.BindlessStorageBufferLayout;
		m_BindlessCombinedImageSamplerLayout = descriptorSetup.BindlessCombinedImageSamplerLayout;
		m_BindlessStorageImageLayout = descriptorSetup.BindlessStorageImageLayout;
		m_BindlessSetLayouts = descriptorSetup.BindlessSetLayouts;

#ifdef GRAVIX_EDITOR_BUILD
		m_ShaderCompiler = CreateRef<ShaderCompiler>();
#endif
	}

	VulkanDevice::~VulkanDevice()
	{
#ifdef GRAVIX_EDITOR_BUILD
		m_ShaderCompiler = nullptr; // Automatically cleaned up by Ref<>
#endif

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

}