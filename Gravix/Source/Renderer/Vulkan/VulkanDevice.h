#pragma once

#include "Renderer/Generic/Device.h"

#ifdef ENGINE_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "Utils/VulkanTypes.h"
#include "Utils/ShaderCompiler.h"
#include "VulkanSwapchain.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Gravix
{

	struct FrameData
	{
		VkCommandPool CommandPool;
		VkCommandBuffer CommandBuffer;

		VkFence RenderFence;
		VkSemaphore SwapchainSemaphore;  // Per-frame: signaled by acquire, waited by submit
	};


	class VulkanDevice : public Device
	{
	public:
		VulkanDevice(const DeviceProperties& deviceProperties);
		virtual ~VulkanDevice();

		virtual DeviceType GetType() const override { return DeviceType::Vulkan; }

		virtual void StartFrame() override;
		virtual void EndFrame() override;
		virtual void WaitIdle() override;

		ShaderCompiler& GetShaderCompiler() { return *m_ShaderCompiler; }

		AllocatedImage CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool useSamples = false, bool mipmapped = false);
		AllocatedImage CreateImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		void DestroyImage(const AllocatedImage& img) { vkDestroyImageView(m_Device, img.ImageView, nullptr); vmaDestroyImage(m_Allocator, img.Image, img.Allocation); }

		AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
		void DestroyBuffer(const AllocatedBuffer& buffer) { vmaDestroyBuffer(m_Allocator, buffer.Buffer, buffer.Allocation); }

		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

		VkInstance GetInstance() const { return m_Instance; }
		VkDevice GetDevice() const { return m_Device; }
		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }

		VkDescriptorSet* GetGlobalDescriptorSets() { return m_BindlessDescriptorSets; }
		VkDescriptorSet GetGlobalDescriptorSet(uint32_t index) const { return m_BindlessDescriptorSets[index]; }
		std::vector<VkDescriptorSetLayout>& GetGlobalDescriptorSetLayouts() { return m_BindlessSetLayouts; }
		VkDescriptorPool GetGlobalDescriptorPool() const { return m_DescriptorPool; }

		VkDescriptorPool GetImGuiDescriptorPool() { return m_ImGuiDescriptorPool; }

		VkImageView GetCurrentSwapchainImageView() const { return m_Swapchain->GetCurrentImageView(); }
		VkImage GetCurrentSwapchainImage() const { return m_Swapchain->GetCurrentImage(); }
		VkImageLayout GetCurrentSwapchainImageLayout() const { return m_SwapchainImageLayout; }
		VkExtent2D GetSwapchainExtent() const { return m_Swapchain->GetExtent(); }
		VkFormat GetSwapchainImageFormat() const { return m_Swapchain->GetImageFormat(); }
		void SetCurrentSwapchainImageLayout(VkImageLayout layout) { m_SwapchainImageLayout = layout; }
		VulkanSwapchain* GetSwapchain() { return m_Swapchain.get(); }

		FrameData& GetCurrentFrameData() { return m_Frames[m_CurrentFrame % FRAME_OVERLAP]; }
		VmaAllocator& GetAllocator() { return m_Allocator; }
	private:
		void InitVulkan(const DeviceProperties& properties);
	

		void InitDescriptorPool();
		void CreateBindlessDescriptorSets();
		void CreateBindlessLayout(VkDescriptorType type, uint32_t count, VkShaderStageFlags stages, VkDescriptorSetLayout* layout);
		void CreateImGuiPool();
		void InitCommandBuffers();
		void InitSyncStructures();
	private:
		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_Device;
		VkSurfaceKHR m_Surface;

		VmaAllocator m_Allocator;

		ShaderCompiler* m_ShaderCompiler;

		VkQueue m_GraphicsQueue;
		uint32_t m_GraphicsQueueFamilyIndex;

		VkQueue m_TransferQueue;
		uint32_t m_TransferQueueFamilyIndex;

		VkFence m_ImmediateFence;
		VkCommandBuffer m_ImmediateCommandBuffer;
		VkCommandPool m_ImmediateCommandPool;
		std::mutex m_ImmediateSubmitMutex;

		VkDescriptorPool m_DescriptorPool;
		VkDescriptorSetLayout m_BindlessStorageBufferLayout;
		VkDescriptorSetLayout m_BindlessCombinedImageSamplerLayout;
		VkDescriptorSetLayout m_BindlessStorageImageLayout;
		std::vector<VkDescriptorSetLayout> m_BindlessSetLayouts;
		VkDescriptorSet m_BindlessDescriptorSets[4]; // 0: Storage Buffers, 1: Sampled Images, 2: Storage Images, 3: Samplers

		VkDescriptorPool m_ImGuiDescriptorPool;

		VkPipelineLayout m_PipelineLayout;

		Scope<VulkanSwapchain> m_Swapchain;
		VkImageLayout m_SwapchainImageLayout;

		FrameData m_Frames[FRAME_OVERLAP];
		uint32_t m_CurrentFrame = 0;

			bool m_Vsync;
		bool m_FrameStarted = false;

#ifdef ENGINE_DEBUG
		bool m_UseValidationLayer = true;
#else 
		bool m_UseValidationLayer = false;
#endif
	};

}