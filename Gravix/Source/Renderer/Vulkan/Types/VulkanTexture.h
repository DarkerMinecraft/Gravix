#pragma once

#include "Renderer/Generic/Types/Texture.h"

#include "Core/UUID.h"

#include "Renderer/Vulkan/VulkanDevice.h"
#include "Renderer/Vulkan/Utils/VulkanTypes.h"

namespace Gravix
{

	class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(Device* device, Buffer data, uint32_t width, uint32_t height, const TextureSpecification& specification);
		virtual ~VulkanTexture2D();

		// Inherited from Texture
		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetMipLevels() const override { return m_MipLevels; }

		virtual void* GetImGuiAttachment() override;
		virtual void DestroyImGuiDescriptor() override;

		virtual UUID GetUUID() override { return m_UUID; }

		virtual bool operator==(const Texture& other) const override 
		{
			const auto* o = dynamic_cast<const VulkanTexture2D*>(&other);
			if (!o)
				return false;

			return m_UUID == o->m_UUID;
		}

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
		void CreateFromData(Buffer data, uint32_t width, uint32_t height, uint32_t channels);
		void CreateVulkanResources(Buffer data, uint32_t dataSize);
		void CreateSampler();
		void Cleanup();

		void CreateMagentaTexture();

		VkFilter ConvertFilter(TextureFilter filter) const;
		VkSamplerAddressMode ConvertWrap(TextureWrap wrap) const;
	private:
		VulkanDevice* m_Device;
		TextureSpecification m_Specification;

		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		uint32_t m_Channels = 0;
		uint32_t m_MipLevels = 1;

		VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;

		UUID m_UUID;

		AllocatedImage m_Image{};
		VkSampler m_Sampler = VK_NULL_HANDLE;
	};

}