#include "pch.h"
#include "VulkanDescriptorSetup.h"

#include "Core/Log.h"
#include "Renderer/Vulkan/VulkanRenderCaps.h"

#include <stdexcept>
#include <algorithm>

namespace Gravix
{

	VulkanDescriptorSetupResult VulkanDescriptorSetup::Initialize(VkDevice device)
	{
		VulkanDescriptorSetupResult result{};

		// Create main descriptor pool
		result.DescriptorPool = CreateMainDescriptorPool(device);

		// Create bindless descriptor sets and layouts
		CreateBindlessDescriptorSets(device, result.DescriptorPool, result);

		// Create ImGui descriptor pool
		result.ImGuiDescriptorPool = CreateImGuiDescriptorPool(device);

		return result;
	}

	VkDescriptorPool VulkanDescriptorSetup::CreateMainDescriptorPool(VkDevice device)
	{
		// Get recommended bindless limits from capabilities
		uint32_t maxSamplers = VulkanRenderCaps::GetRecommendedBindlessSamplers();
		uint32_t maxSampledImages = VulkanRenderCaps::GetRecommendedBindlessSampledImages();
		uint32_t maxStorageImages = VulkanRenderCaps::GetRecommendedBindlessStorageImages();
		uint32_t maxStorageBuffers = VulkanRenderCaps::GetRecommendedBindlessStorageBuffers();
		uint32_t maxUniformBuffers = VulkanRenderCaps::GetMaxDescriptorSetUniformBuffers();

		// Apply reasonable limits for uniform buffers (not typically bindless)
		maxUniformBuffers = std::min(maxUniformBuffers, 1000u);

		GX_CORE_INFO("Creating Descriptor Pool with:");
		GX_CORE_INFO("     Samplers: {0}", maxSamplers);
		GX_CORE_INFO("     Sampled Images: {0}", maxSampledImages);
		GX_CORE_INFO("     Storage Images: {0}", maxStorageImages);
		GX_CORE_INFO("     Storage Buffers: {0}", maxStorageBuffers);
		GX_CORE_INFO("     Uniform Buffers: {0}", maxUniformBuffers);

		// Define pool sizes for each descriptor type
		std::vector<VkDescriptorPoolSize> poolSizes = {
			// Bindless resource types
			{ VK_DESCRIPTOR_TYPE_SAMPLER, maxSamplers },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxSampledImages },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxSampledImages }, // Alternative binding method
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxStorageImages },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxStorageBuffers },

			// Regular descriptor types (for non-bindless descriptors)
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxUniformBuffers },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },  // For dynamic uniforms
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },  // For dynamic storage

			// Ray tracing descriptors (if supported)
			{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1000 }
		};

		// Calculate total descriptor sets needed
		uint32_t maxBoundSets = VulkanRenderCaps::GetMaxBoundDescriptorSets();
		uint32_t maxDescriptorSets = std::max(maxBoundSets * 2, 100u); // Reduced from 10x to 2x (was 1000 min, now 100)

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT |
			VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT; // Required for bindless
		poolInfo.maxSets = maxDescriptorSets;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();

		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
		if (result != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to create descriptor pool! Error: {0}", static_cast<int>(result));
			throw std::runtime_error("Failed to create descriptor pool");
		}

		GX_CORE_INFO("Descriptor Pool created successfully with {0} max sets", maxDescriptorSets);

		return descriptorPool;
	}

	VkDescriptorPool VulkanDescriptorSetup::CreateImGuiDescriptorPool(VkDevice device)
	{
		// Reduced from 1000 to 100 for better memory usage
		// Increase if you have many ImGui textures/windows
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 100 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100 }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 100;  // ImGui creates one set per texture (reduced from 1000)
		pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
		pool_info.pPoolSizes = pool_sizes;

		VkDescriptorPool imguiPool = VK_NULL_HANDLE;
		VkResult result = vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiPool);
		if (result != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to create ImGui descriptor pool! Error: {0}", static_cast<int>(result));
			throw std::runtime_error("Failed to create ImGui descriptor pool");
		}

		GX_CORE_INFO("ImGui Descriptor Pool created successfully");

		return imguiPool;
	}

	void VulkanDescriptorSetup::CreateBindlessDescriptorSets(VkDevice device, VkDescriptorPool pool, VulkanDescriptorSetupResult& result)
	{
		// Query max bindless counts
		uint32_t maxSamplers = VulkanRenderCaps::GetRecommendedBindlessSamplers();
		uint32_t maxSampledImages = VulkanRenderCaps::GetRecommendedBindlessSampledImages();
		uint32_t maxStorageImages = VulkanRenderCaps::GetRecommendedBindlessStorageImages();
		uint32_t maxStorageBuffers = VulkanRenderCaps::GetRecommendedBindlessStorageBuffers();

		// For combined image samplers, take the smaller of samplers/images (safe bound)
		uint32_t maxCombinedImageSamplers = std::min(maxSamplers, maxSampledImages);

		// Create bindless layouts: each set holds 1 type with max descriptors
		CreateBindlessLayout(device, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxStorageBuffers, VK_SHADER_STAGE_ALL, &result.BindlessStorageBufferLayout);
		CreateBindlessLayout(device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxCombinedImageSamplers, VK_SHADER_STAGE_ALL, &result.BindlessCombinedImageSamplerLayout);
		CreateBindlessLayout(device, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxStorageImages, VK_SHADER_STAGE_ALL, &result.BindlessStorageImageLayout);

		result.BindlessSetLayouts = {
			result.BindlessStorageBufferLayout,       // set 0
			result.BindlessCombinedImageSamplerLayout,// set 1
			result.BindlessStorageImageLayout         // set 2
		};

		// Variable counts for allocation
		uint32_t variableCounts[] = { maxStorageBuffers, maxCombinedImageSamplers, maxStorageImages };

		VkDescriptorSetVariableDescriptorCountAllocateInfo variableInfo{};
		variableInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
		variableInfo.descriptorSetCount = 3;
		variableInfo.pDescriptorCounts = variableCounts;

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = &variableInfo;
		allocInfo.descriptorPool = pool;
		allocInfo.descriptorSetCount = 3;
		allocInfo.pSetLayouts = result.BindlessSetLayouts.data();

		if (vkAllocateDescriptorSets(device, &allocInfo, result.BindlessDescriptorSets) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate bindless descriptor sets!");
		}

		GX_CORE_INFO("Bindless descriptor sets created with max bindings:");
		GX_CORE_INFO("   Storage Buffers:        {0}", maxStorageBuffers);
		GX_CORE_INFO("   Combined Image Samplers:{0}", maxCombinedImageSamplers);
		GX_CORE_INFO("   Storage Images:         {0}", maxStorageImages);
	}

	void VulkanDescriptorSetup::CreateBindlessLayout(VkDevice device, VkDescriptorType type, uint32_t count, VkShaderStageFlags stages, VkDescriptorSetLayout* layout)
	{
		VkDescriptorSetLayoutBinding bindlessBinding = {};
		bindlessBinding.binding = 0; // ALWAYS 0 inside the set
		bindlessBinding.descriptorType = type;
		bindlessBinding.descriptorCount = count;
		bindlessBinding.stageFlags = stages;
		bindlessBinding.pImmutableSamplers = nullptr;

		VkDescriptorBindingFlags bindingFlags =
			VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

		VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {};
		bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		bindingFlagsInfo.bindingCount = 1;
		bindingFlagsInfo.pBindingFlags = &bindingFlags;

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.pNext = &bindingFlagsInfo;
		layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &bindlessBinding;

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, layout) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create bindless descriptor set layout!");
		}
	}

}
