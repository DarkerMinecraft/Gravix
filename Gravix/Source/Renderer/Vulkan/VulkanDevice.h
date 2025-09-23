#pragma once

#include "Renderer/Generic/Device.h"

#ifdef ENGINE_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Gravix 
{

	struct FrameData 
	{
		VkCommandPool CommandPool;
		VkCommandBuffer CommandBuffer;

		VkSemaphore SwapchainSemaphore, RenderSemaphore;
		VkFence RenderFence;
	};


	class VulkanDevice : public Device
	{
	public:
		VulkanDevice(const DeviceProperties& deviceProperties);
		virtual ~VulkanDevice();

		virtual DeviceType GetType() const override { return DeviceType::Vulkan; }

		virtual void StartFrame() override;
		virtual void EndFrame() override;

		VkInstance GetInstance() const { return m_Instance; }
		VkDevice GetDevice() const { return m_Device; }
		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }

		VkDescriptorSet* GetGlobalDescriptorSets() { return m_BindlessDescriptorSets; }
		VkDescriptorPool GetGlobalDescriptorPool() const { return m_DescriptorPool; }

		FrameData& GetCurrentFrameData() { return m_Frames[m_CurrentFrame % FRAME_OVERLAP]; }
	private:
		void InitVulkan(const DeviceProperties& properties);
		void CreateSwapchain(uint32_t width, uint32_t height, bool vSync);
		void DestroySwapchain();
		void RecreateSwapchain(uint32_t width, uint32_t height, bool vSync);

		void InitDescriptorPool();
		void CreateBindlessDescriptorSets();
		void CreateBindlessLayout(uint32_t binding, VkDescriptorType type, uint32_t count,
			VkShaderStageFlags stages, VkDescriptorSetLayout* layout);
		void InitCommandBuffers();
		void InitSyncStructures();
	private:
		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_Device;
		VkSurfaceKHR m_Surface;
		
		VmaAllocator m_Allocator;

		VkQueue m_GraphicsQueue;
		uint32_t m_GraphicsQueueFamilyIndex;

		VkDescriptorPool m_DescriptorPool;
		VkDescriptorSetLayout m_BindlessStorageBufferLayout;
		VkDescriptorSetLayout m_BindlessSampledImageLayout;
		VkDescriptorSetLayout m_BindlessStorageImageLayout;
		VkDescriptorSetLayout m_BindlessSamplerLayout;
		VkDescriptorSet m_BindlessDescriptorSets[4]; // 0: Storage Buffers, 1: Sampled Images, 2: Storage Images, 3: Samplers

		VkPipelineLayout m_PipelineLayout;

		VkSwapchainKHR m_Swapchain;
		VkFormat m_SwapchainImageFormat;
		VkImageLayout m_SwapchainImageLayout; 

		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;
		VkExtent2D m_SwapchainExtent;

		FrameData m_Frames[FRAME_OVERLAP];
		uint32_t m_CurrentFrame = 0;

		uint32_t m_SwapchainImageIndex = 0;
		bool m_Vsync;

#ifdef ENGINE_DEBUG
		bool m_UseValidationLayer = true;
#else 
		bool m_UseValidationLayer = false;
#endif
	};

}