#include "pch.h"
#include "VulkanSwapchain.h"
#include "Utils/VulkanInitializers.h"
#include "Core/Log.h"

#include <VkBootstrap.h>

namespace Gravix
{

	VulkanSwapchain::VulkanSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
		: m_Device(device), m_PhysicalDevice(physicalDevice), m_Surface(surface)
	{
	}

	VulkanSwapchain::~VulkanSwapchain()
	{
		Destroy();
	}

	bool VulkanSwapchain::Create(uint32_t width, uint32_t height, bool vSync)
	{
		// Don't create swapchain with zero dimensions (window minimized)
		if (width == 0 || height == 0)
		{
			GX_CORE_WARN("Cannot create swapchain with zero dimensions (width={}, height={}). Window may be minimized.", width, height);
			return false;
		}

		m_VSync = vSync;

		vkb::SwapchainBuilder swapchainBuilder{ m_PhysicalDevice, m_Device, m_Surface };

		m_ImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.set_desired_format(VkSurfaceFormatKHR{ .format = m_ImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			.set_desired_extent(width, height)
			.set_desired_present_mode(vSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		m_Extent = vkbSwapchain.extent;
		m_Swapchain = vkbSwapchain.swapchain;
		m_Images = vkbSwapchain.get_images().value();
		m_ImageViews = vkbSwapchain.get_image_views().value();

		// Create synchronization structures
		CreateSyncStructures();

		GX_CORE_INFO("Created swapchain: {}x{}, {} images, VSync: {}", width, height, m_Images.size(), vSync);

		return true;
	}

	void VulkanSwapchain::Destroy()
	{
		if (m_Swapchain != VK_NULL_HANDLE)
		{
			// Destroy sync structures
			DestroySyncStructures();

			// Destroy image views
			for (auto imageView : m_ImageViews)
			{
				vkDestroyImageView(m_Device, imageView, nullptr);
			}
			m_ImageViews.clear();

			// Destroy swapchain
			vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
			m_Swapchain = VK_NULL_HANDLE;

			m_Images.clear();
		}
	}

	bool VulkanSwapchain::Recreate(uint32_t width, uint32_t height, bool vSync)
	{
		// Don't recreate swapchain with zero dimensions (window minimized)
		if (width == 0 || height == 0)
		{
			GX_CORE_WARN("Cannot recreate swapchain with zero dimensions (width={}, height={}). Window may be minimized.", width, height);
			return false;
		}

		// Wait for GPU to finish before destroying swapchain resources
		vkDeviceWaitIdle(m_Device);

		// Destroy old resources
		DestroySyncStructures();

		for (auto imageView : m_ImageViews)
		{
			vkDestroyImageView(m_Device, imageView, nullptr);
		}
		m_ImageViews.clear();

		// Build new swapchain
		m_VSync = vSync;
		vkb::SwapchainBuilder swapchainBuilder{ m_PhysicalDevice, m_Device, m_Surface };

		m_ImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			.set_desired_format(VkSurfaceFormatKHR{ .format = m_ImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			.set_desired_extent(width, height)
			.set_old_swapchain(m_Swapchain)
			.set_desired_present_mode(vSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		// Destroy old swapchain
		vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);

		// Store new swapchain
		m_Extent = vkbSwapchain.extent;
		m_Swapchain = vkbSwapchain.swapchain;
		m_Images = vkbSwapchain.get_images().value();
		m_ImageViews = vkbSwapchain.get_image_views().value();

		// Recreate sync structures
		CreateSyncStructures();

		GX_CORE_INFO("Recreated swapchain: {}x{}, {} images, VSync: {}", width, height, m_Images.size(), vSync);

		return true;
	}

	VkResult VulkanSwapchain::AcquireNextImage(VkSemaphore signalSemaphore, uint32_t* imageIndex)
	{
		VkResult result = vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, signalSemaphore, nullptr, imageIndex);

		if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)
		{
			m_CurrentImageIndex = *imageIndex;
		}

		return result;
	}

	VkResult VulkanSwapchain::Present(VkQueue queue, uint32_t imageIndex)
	{
		// Get the current render semaphore for this image
		auto& syncData = m_SwapchainSyncData[imageIndex];
		VkSemaphore renderSemaphore = syncData.RenderSemaphores[syncData.SemaphoreIndex];

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.pSwapchains = &m_Swapchain;
		presentInfo.swapchainCount = 1;
		presentInfo.pWaitSemaphores = &renderSemaphore;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pImageIndices = &imageIndex;

		VkResult result = vkQueuePresentKHR(queue, &presentInfo);

		// Rotate to next semaphore for this swapchain image
		syncData.SemaphoreIndex = (syncData.SemaphoreIndex + 1) % FRAME_OVERLAP;

		return result;
	}

	void VulkanSwapchain::CreateSyncStructures()
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = VulkanInitializers::SemaphoreCreateInfo();

		// Create per-swapchain-image, per-frame present semaphores
		m_SwapchainSyncData.resize(m_Images.size());
		for (size_t i = 0; i < m_Images.size(); i++)
		{
			for (uint32_t j = 0; j < FRAME_OVERLAP; j++)
			{
				vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_SwapchainSyncData[i].RenderSemaphores[j]);
			}
			m_SwapchainSyncData[i].SemaphoreIndex = 0;
		}
	}

	void VulkanSwapchain::DestroySyncStructures()
	{
		for (auto& syncData : m_SwapchainSyncData)
		{
			for (uint32_t j = 0; j < FRAME_OVERLAP; j++)
			{
				if (syncData.RenderSemaphores[j] != VK_NULL_HANDLE)
				{
					vkDestroySemaphore(m_Device, syncData.RenderSemaphores[j], nullptr);
					syncData.RenderSemaphores[j] = VK_NULL_HANDLE;
				}
			}
		}
		m_SwapchainSyncData.clear();
	}

}
