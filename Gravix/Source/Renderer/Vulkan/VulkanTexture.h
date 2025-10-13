#pragma once

#include "Renderer/Generic/Texture.h"
#include "Utils/VulkanTypes.h"

#include "VulkanDevice.h"

namespace Gravix
{

	class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(Device* device, const std::filesystem::path& path, const TextureSpecification& specification);
		VulkanTexture2D(Device* device, void* data, uint32_t width, uint32_t height, const TextureSpecification& specification);
		virtual ~VulkanTexture2D();

		// Inherited from Texture
		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetMipLevels() const override { return m_MipLevels; }

		// Vulkan-specific methods
		VkImage GetVkImage() const { return m_Image.Image; }
		VkImageView GetVkImageView() const { return m_Image.ImageView; }
		VkSampler GetVkSampler() const { return m_Sampler; }
		AllocatedImage GetAllocatedImage() const { return m_Image; }

		/**
		 * @brief Get texture descriptor info for shader binding
		 */
		VkDescriptorImageInfo GetDescriptorInfo() const;
	private:
		void LoadFromFile(const std::filesystem::path& path);
		void CreateFromData(const void* data, uint32_t width, uint32_t height, uint32_t channels);
		void CreateVulkanResources(const void* data, uint32_t dataSize);
		void CreateSampler();
		void Cleanup();

		VkFilter ConvertFilter(TextureFilter filter) const;
		VkSamplerAddressMode ConvertWrap(TextureWrap wrap) const;
	private:
		VulkanDevice* m_Device;
		TextureSpecification m_Specification;

		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		uint32_t m_Channels = 0;
		uint32_t m_MipLevels = 1;

		AllocatedImage m_Image{};
		VkSampler m_Sampler = VK_NULL_HANDLE;
	};

}