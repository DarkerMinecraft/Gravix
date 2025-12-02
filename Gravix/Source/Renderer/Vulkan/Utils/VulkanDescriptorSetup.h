#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace Gravix
{

	struct VulkanDescriptorSetupResult
	{
		VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
		VkDescriptorPool ImGuiDescriptorPool = VK_NULL_HANDLE;

		// Bindless descriptor sets and layouts
		VkDescriptorSet BindlessDescriptorSets[3] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
		VkDescriptorSetLayout BindlessStorageBufferLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout BindlessCombinedImageSamplerLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout BindlessStorageImageLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSetLayout> BindlessSetLayouts;
	};

	class VulkanDescriptorSetup
	{
	public:
		static VulkanDescriptorSetupResult Initialize(VkDevice device);

	private:
		static VkDescriptorPool CreateMainDescriptorPool(VkDevice device);
		static VkDescriptorPool CreateImGuiDescriptorPool(VkDevice device);
		static void CreateBindlessDescriptorSets(VkDevice device, VkDescriptorPool pool, VulkanDescriptorSetupResult& result);
		static void CreateBindlessLayout(VkDevice device, VkDescriptorType type, uint32_t count, VkShaderStageFlags stages, VkDescriptorSetLayout* layout);
	};

}
