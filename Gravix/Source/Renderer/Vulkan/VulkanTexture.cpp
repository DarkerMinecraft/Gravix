#include "pch.h"
#include "VulkanTexture.h"

#include "Renderer/Vulkan/Utils/VulkanUtils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Gravix
{
	VulkanTexture2D::VulkanTexture2D(Device* device, const std::filesystem::path& path, const TextureSpecification& specification)
		: m_Device(static_cast<VulkanDevice*>(device))
		, m_Specification(specification)
	{
		LoadFromFile(path);
	}

	VulkanTexture2D::VulkanTexture2D(Device* device, void* data, uint32_t width, uint32_t height, const TextureSpecification& specification)
		: m_Device(static_cast<VulkanDevice*>(device))
		, m_Specification(specification)
		, m_Width(width)
		, m_Height(height)
		, m_Channels(4) // Assume RGBA
	{
		CreateFromData(data, width, height, m_Channels);
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		Cleanup();
	}

	void VulkanTexture2D::LoadFromFile(const std::filesystem::path& path)
	{
		// Force RGBA format for consistency
		stbi_set_flip_vertically_on_load(false); // Vulkan has inverted Y compared to OpenGL

		int width, height, channels;
		stbi_uc* data = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

		if (!data)
		{
			GX_CORE_ERROR("Failed to load texture: {0} - {1}", path.string(), stbi_failure_reason());

			// Create a default 1x1 magenta texture to indicate missing texture
			m_Width = 1;
			m_Height = 1;
			m_Channels = 4;
			uint8_t magentaPixel[4] = { 255, 0, 255, 255 }; // Magenta RGBA
			CreateFromData(magentaPixel, 1, 1, 4);
			return;
		}

		m_Width = static_cast<uint32_t>(width);
		m_Height = static_cast<uint32_t>(height);
		m_Channels = 4; // We forced RGBA above

		CreateFromData(data, m_Width, m_Height, m_Channels);

		// Free stbi memory
		stbi_image_free(data);
	}

	void VulkanTexture2D::CreateFromData(const void* data, uint32_t width, uint32_t height, uint32_t channels)
	{
		if (!data)
		{
			return;
		}

		m_Width = width;
		m_Height = height;
		m_Channels = channels;

		// Calculate mip levels if mipmaps are enabled
		if (m_Specification.GenerateMipmaps)
		{
			m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_Width, m_Height)))) + 1;
		}
		else
		{
			m_MipLevels = 1;
		}

		CreateVulkanResources(data, m_Width * m_Height * 4); // Assume RGBA = 4 bytes per pixel
		CreateSampler();
	}

	void VulkanTexture2D::CreateVulkanResources(const void* data, uint32_t dataSize)
	{
		VkExtent3D imageExtent = { m_Width, m_Height, 1 };
		VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM; // Standard RGBA format

		// Create Vulkan image with transfer usage for uploading data
		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		if (m_Specification.GenerateMipmaps)
		{
			usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // Needed for mipmap generation
		}

		// Use the device's CreateImage method with data - this handles mipmap generation internally
		m_Image = m_Device->CreateImage(const_cast<void*>(data), imageExtent, imageFormat, usage, m_Specification.GenerateMipmaps);

		if (m_Image.Image == VK_NULL_HANDLE)
		{
			GX_CORE_ERROR("Failed to create Vulkan image for texture: {0}", m_Specification.DebugName);
			return;
		}

		m_Device->ImmediateSubmit([&](VkCommandBuffer cmd)
		{
			VulkanUtils::TransitionImage(cmd, m_Image.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		});

	}

	void VulkanTexture2D::CreateSampler()
	{
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = ConvertFilter(m_Specification.MagFilter);
		samplerInfo.minFilter = ConvertFilter(m_Specification.MinFilter);
		samplerInfo.addressModeU = ConvertWrap(m_Specification.WrapS);
		samplerInfo.addressModeV = ConvertWrap(m_Specification.WrapT);
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		// Anisotropic filtering
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16.0f; // TODO: Get from device capabilities

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		// Mipmap settings
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = static_cast<float>(m_MipLevels);

		VkResult result = vkCreateSampler(m_Device->GetDevice(), &samplerInfo, nullptr, &m_Sampler);
		if (result != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to create texture sampler for '{0}': {1}", m_Specification.DebugName, static_cast<int>(result));
			return;
		}
	}

	VkDescriptorImageInfo VulkanTexture2D::GetDescriptorInfo() const
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = m_Image.ImageView;
		imageInfo.sampler = m_Sampler;
		return imageInfo;
	}

	void VulkanTexture2D::Cleanup()
	{
		vkDeviceWaitIdle(m_Device->GetDevice());

		if (m_Sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(m_Device->GetDevice(), m_Sampler, nullptr);
			m_Sampler = VK_NULL_HANDLE;
		}

		if (m_Image.Image != VK_NULL_HANDLE)
		{
			m_Device->DestroyImage(m_Image);
			m_Image = {};
		}
	}

	VkFilter VulkanTexture2D::ConvertFilter(TextureFilter filter) const
	{
		switch (filter)
		{
		case TextureFilter::Nearest: return VK_FILTER_NEAREST;
		case TextureFilter::Linear:  return VK_FILTER_LINEAR;
		default:                    return VK_FILTER_LINEAR;
		}
	}

	VkSamplerAddressMode VulkanTexture2D::ConvertWrap(TextureWrap wrap) const
	{
		switch (wrap)
		{
		case TextureWrap::Repeat:       return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case TextureWrap::ClampToEdge:  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case TextureWrap::ClampToBorder:return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		default:                       return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
	}
}