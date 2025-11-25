#pragma once

#include "Renderer/Generic/Device.h"

#include <vulkan/vulkan.h>
#include <vector>

namespace Gravix
{
	struct SwapchainSyncData
	{
		VkSemaphore RenderSemaphores[FRAME_OVERLAP];  // Per-swapchain-image, per-frame: signaled by submit, waited by present
		uint32_t SemaphoreIndex = 0;  // Track which semaphore to use next
	};

	class VulkanSwapchain
	{
	public:
		VulkanSwapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
		~VulkanSwapchain();

		// Create/Destroy/Recreate swapchain
		bool Create(uint32_t width, uint32_t height, bool vSync);
		void Destroy();
		bool Recreate(uint32_t width, uint32_t height, bool vSync);

		// Frame operations
		VkResult AcquireNextImage(VkSemaphore signalSemaphore, uint32_t* imageIndex);
		VkResult Present(VkQueue queue, uint32_t imageIndex);

		// Get current semaphore for rendering to acquired image
		VkSemaphore GetCurrentRenderSemaphore() const { return m_SwapchainSyncData[m_CurrentImageIndex].RenderSemaphores[m_SwapchainSyncData[m_CurrentImageIndex].SemaphoreIndex]; }
		void AdvanceSemaphoreIndex() { m_SwapchainSyncData[m_CurrentImageIndex].SemaphoreIndex = (m_SwapchainSyncData[m_CurrentImageIndex].SemaphoreIndex + 1) % FRAME_OVERLAP; }

		// Getters
		VkSwapchainKHR GetHandle() const { return m_Swapchain; }
		VkFormat GetImageFormat() const { return m_ImageFormat; }
		VkExtent2D GetExtent() const { return m_Extent; }
		const std::vector<VkImage>& GetImages() const { return m_Images; }
		const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }
		VkImage GetCurrentImage() const { return m_Images[m_CurrentImageIndex]; }
		VkImageView GetCurrentImageView() const { return m_ImageViews[m_CurrentImageIndex]; }
		uint32_t GetCurrentImageIndex() const { return m_CurrentImageIndex; }

	private:
		void CreateSyncStructures();
		void DestroySyncStructures();

	private:
		VkDevice m_Device;
		VkPhysicalDevice m_PhysicalDevice;
		VkSurfaceKHR m_Surface;

		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
		VkExtent2D m_Extent = {};

		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;
		std::vector<SwapchainSyncData> m_SwapchainSyncData;

		uint32_t m_CurrentImageIndex = 0;
		bool m_VSync = false;
	};
}
