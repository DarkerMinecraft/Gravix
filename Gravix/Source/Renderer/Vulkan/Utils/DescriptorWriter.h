#pragma once

#include <vulkan/vulkan.h>

namespace Gravix
{
	class DescriptorWriter
	{
	public:
		DescriptorWriter(VkDescriptorSetLayout layout, VkDescriptorPool pool);
		DescriptorWriter() : m_Layout(VK_NULL_HANDLE), m_Pool(VK_NULL_HANDLE) {}  // Default constructor

		DescriptorWriter& WriteBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
		DescriptorWriter& WriteImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout);
		DescriptorWriter& WriteImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout, VkDescriptorType type, uint32_t arrayIndex = 0);
		DescriptorWriter& WriteImage(uint32_t binding, uint32_t index, VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout);
		DescriptorWriter& WriteImage(uint32_t binding, VkImageView imageView, VkImageLayout imageLayout);
		DescriptorWriter& WriteImage(uint32_t binding, VkImageView imageView, VkImageLayout imageLayout, VkDescriptorType type);

		void Overwrite(VkDevice device, VkDescriptorSet set);
		void UpdateSet(VkDevice device, VkDescriptorSet set) { Overwrite(device, set); }  // Alias for compatibility

	private:
		VkDescriptorSetLayout m_Layout;
		VkDescriptorPool m_Pool;
		std::vector<VkWriteDescriptorSet> m_Writes;
		std::vector<VkDescriptorImageInfo> m_ImageInfos;  
		std::vector<VkDescriptorBufferInfo> m_BufferInfos; 
	};
}