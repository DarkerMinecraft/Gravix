#include "pch.h"
#include "VulkanMaterial.h"
#include "VulkanFramebuffer.h"
#include "VulkanTexture.h"

#include "Renderer/Vulkan/Utils/DescriptorWriter.h"
#include "Renderer/Vulkan/Utils/VulkanUtils.h"
#include "Core/Application.h"

namespace Gravix
{

	static VkFormat ShaderDataTypeToVulkanFormat(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::Float:     return VK_FORMAT_R32_SFLOAT;
		case ShaderDataType::Float2:    return VK_FORMAT_R32G32_SFLOAT;
		case ShaderDataType::Float3:    return VK_FORMAT_R32G32B32_SFLOAT;
		case ShaderDataType::Float4:    return VK_FORMAT_R32G32B32A32_SFLOAT;
		case ShaderDataType::Int:       return VK_FORMAT_R32_SINT;
		case ShaderDataType::Int2:      return VK_FORMAT_R32G32_SINT;
		case ShaderDataType::Int3:      return VK_FORMAT_R32G32B32_SINT;
		case ShaderDataType::Int4:      return VK_FORMAT_R32G32B32A32_SINT;
		case ShaderDataType::Bool:      return VK_FORMAT_R8_UINT;
		default: return VK_FORMAT_UNDEFINED;
		}
	}

	static VkShaderStageFlagBits ShaderStageToVulkanShaderStage(ShaderStage stage)
	{
		switch (stage)
		{
		case ShaderStage::Vertex:   return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderStage::Compute:  return VK_SHADER_STAGE_COMPUTE_BIT;
		case ShaderStage::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderStage::All:      return VK_SHADER_STAGE_ALL;
		default:                    return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		}
	}

	static VkShaderStageFlagBits ShaderStageToVulkanShaderStageBit(std::vector<ShaderStage> avaliableStages)
	{
		VkShaderStageFlagBits flags = VkShaderStageFlagBits(0);
		for (auto stage : avaliableStages)
		{
			VkShaderStageFlagBits stageFlag = ShaderStageToVulkanShaderStage(stage);
			flags = static_cast<VkShaderStageFlagBits>(flags | stageFlag);
		}
		return flags;
	}

	static bool ShouldRegeneratePipelineCache(VkPhysicalDevice device, const std::vector<uint8_t>& cachedData) 
	{
		if (cachedData.size() < sizeof(VkPipelineCacheHeaderVersionOne))
			return true;
		const auto* cacheHeader = reinterpret_cast<const VkPipelineCacheHeaderVersionOne*>(cachedData.data());
		VkPhysicalDeviceProperties deviceProps;
		vkGetPhysicalDeviceProperties(device, &deviceProps);
		return (cacheHeader->headerVersion != VK_PIPELINE_CACHE_HEADER_VERSION_ONE) ||
			(cacheHeader->vendorID != deviceProps.vendorID) ||
			(cacheHeader->deviceID != deviceProps.deviceID) ||
			(memcmp(cacheHeader->pipelineCacheUUID, deviceProps.pipelineCacheUUID, VK_UUID_SIZE) != 0);
	}

	VulkanMaterial::VulkanMaterial(Device* device, const MaterialSpecification& spec)
		: m_Device(static_cast<VulkanDevice*>(device)), m_DebugName(spec.DebugName)
	{
		std::filesystem::path materialCache = Project::GetLibraryDirectory() / (spec.DebugName + ".cache");	
		GetShaderCache(spec.ShaderFilePath, materialCache);
		CreateMaterial(spec);
		SaveShaderCache(spec.ShaderFilePath, materialCache);
	}

	VulkanMaterial::VulkanMaterial(Device* device, const std::string& debugName, const std::filesystem::path& shaderFilePath)
		: m_Device(static_cast<VulkanDevice*>(device)), m_DebugName(debugName)
	{
		std::filesystem::path materialCache = Project::GetLibraryDirectory() / (debugName + ".cache");
		GetShaderCache(shaderFilePath, materialCache);
		CreateMaterial(debugName, shaderFilePath);
		SaveShaderCache(shaderFilePath, materialCache);

		m_IsCompute = true;
	}

	VulkanMaterial::~VulkanMaterial()
	{
		for (auto& shaderModule : m_ShaderModules)
		{
			vkDestroyShaderModule(m_Device->GetDevice(), shaderModule, nullptr);
		}

		vkDestroyPipelineCache(m_Device->GetDevice(), m_PipelineCache, nullptr);
		vkDestroyPipelineLayout(m_Device->GetDevice(), m_PipelineLayout, nullptr);
		vkDestroyPipeline(m_Device->GetDevice(), m_Pipeline, nullptr);
	}

	void VulkanMaterial::Bind(VkCommandBuffer cmd, void* pushConstants)
	{
		if (m_Pipeline == VK_NULL_HANDLE)
			return;

		vkCmdBindPipeline(cmd, m_IsCompute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
		vkCmdBindDescriptorSets(cmd, m_IsCompute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
			0, static_cast<uint32_t>(m_Device->GetGlobalDescriptorSetLayouts().size()), m_Device->GetGlobalDescriptorSets(), 0, nullptr);
		if (m_PushConstantSize > 0 && pushConstants != nullptr)
			vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_ALL, 0, m_PushConstantSize, pushConstants);

	}

	void VulkanMaterial::Dispatch(VkCommandBuffer cmd, uint32_t width, uint32_t height)
	{
		if (m_Pipeline == VK_NULL_HANDLE || !m_IsCompute)
			return;

		auto& dispatchInfo = m_Reflection.GetComputeDispatch();
		vkCmdDispatch(cmd, std::ceil(width / dispatchInfo.LocalSizeX), std::ceil(height / dispatchInfo.LocalSizeY), dispatchInfo.LocalSizeZ);
	}

	void VulkanMaterial::BindResource(VkCommandBuffer cmd, uint32_t binding, Framebuffer* buffer, uint32_t index, bool sampler)
	{
		VulkanFramebuffer* framebuffer = static_cast<VulkanFramebuffer*>(buffer);
		if (!sampler)
		{
			framebuffer->TransitionToLayout(cmd, index, VK_IMAGE_LAYOUT_GENERAL);

			DescriptorWriter writer(m_Device->GetGlobalDescriptorSetLayouts()[2], m_Device->GetGlobalDescriptorPool());
			writer.WriteImage(binding, framebuffer->GetImage(index).ImageView, VK_IMAGE_LAYOUT_GENERAL);

			writer.Overwrite(m_Device->GetDevice(), m_Device->GetGlobalDescriptorSet(2));
		}
	}

	void VulkanMaterial::BindResource(VkCommandBuffer cmd, uint32_t binding, Texture2D* texture)
	{
		VulkanTexture2D* tex = static_cast<VulkanTexture2D*>(texture);
		DescriptorWriter writer(m_Device->GetGlobalDescriptorSetLayouts()[1], m_Device->GetGlobalDescriptorPool());

		writer.WriteImage(binding, tex->GetVkImageView(), tex->GetVkSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		writer.Overwrite(m_Device->GetDevice(), m_Device->GetGlobalDescriptorSet(1));
	}

	void VulkanMaterial::BindResource(VkCommandBuffer cmd, uint32_t binding, uint32_t index, Texture2D* texture)
	{
		VulkanTexture2D* tex = static_cast<VulkanTexture2D*>(texture);
		DescriptorWriter writer(m_Device->GetGlobalDescriptorSetLayouts()[1], m_Device->GetGlobalDescriptorPool());

		writer.WriteImage(binding, index, tex->GetVkImageView(), tex->GetVkSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		writer.Overwrite(m_Device->GetDevice(), m_Device->GetGlobalDescriptorSet(1));
	}

	void VulkanMaterial::GetShaderCache(const std::filesystem::path& shaderFilePath, const std::filesystem::path& materialCacheFile)
	{
		if (std::filesystem::exists(materialCacheFile))
		{
			MaterialSerializer deserializer(&m_SerializedShaderData);
			deserializer.Deserialize(shaderFilePath, materialCacheFile);

			m_ShouldRegenerateShaderCache = deserializer.IsModified();
			m_ShouldRegeneratePipelineCache = ShouldRegeneratePipelineCache(m_Device->GetPhysicalDevice(), m_SerializedShaderData.PipelineCache);
			if (m_ShouldRegenerateShaderCache)
				m_ShouldRegeneratePipelineCache = true;
		}
		else
		{
			m_ShouldRegenerateShaderCache = true;
			m_ShouldRegeneratePipelineCache = true;
		}
	}

	void VulkanMaterial::CreateMaterial(const MaterialSpecification& spec)
	{
		SpinShader(spec.ShaderFilePath);
		CreatePipelineLayout();
		CreateGraphicsPipeline(spec);
	}

	void VulkanMaterial::CreateMaterial(const std::string& debugName, const std::filesystem::path& shaderFilePath)
	{
		SpinShader(shaderFilePath);
		CreatePipelineLayout();
		CreateComputePipeline();
	}

	void VulkanMaterial::SaveShaderCache(const std::filesystem::path& shaderFilePath, const std::filesystem::path& materialCacheFile)
	{
		if (m_ShouldRegeneratePipelineCache || m_ShouldRegenerateShaderCache)
		{
			m_SerializedShaderData.Reflection = m_Reflection;

			MaterialSerializer serializer(&m_SerializedShaderData);
			serializer.Serialize(shaderFilePath, materialCacheFile);
		}
	}

	void VulkanMaterial::SpinShader(const std::filesystem::path& shaderFilePath)
	{
		std::vector<std::vector<uint32_t>> spirv;
		if (m_ShouldRegenerateShaderCache)
		{
			if (m_Device->GetShaderCompiler().CompileShader(shaderFilePath, &spirv, &m_Reflection))
			{
				GX_CORE_INFO("Successfully compiled shader: {0}", shaderFilePath.string());
				for (uint32_t i = 0; i < spirv.size(); i++)
				{
					VkShaderModuleCreateInfo createInfo{};
					createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
					createInfo.codeSize = spirv[i].size() * sizeof(uint32_t);
					createInfo.pCode = spirv[i].data();
					VkShaderModule shaderModule;
					if (vkCreateShaderModule(m_Device->GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
					{
						GX_CORE_ERROR("Failed to create shader module for: {0}", shaderFilePath.string());
						return;
					}
					m_ShaderModules.push_back(shaderModule);
				}
				m_SerializedShaderData.SpirvCode = spirv;
				m_SerializedShaderData.Reflection = m_Reflection;
			}
			else
			{
				GX_CORE_ERROR("Failed to compile shader: {0}", shaderFilePath.string());
			}
		}
		else 
		{
			for (uint32_t i = 0; i < m_SerializedShaderData.SpirvCode.size(); i++) 
			{
				VkShaderModuleCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				createInfo.codeSize = m_SerializedShaderData.SpirvCode[i].size() * sizeof(uint32_t);
				createInfo.pCode = m_SerializedShaderData.SpirvCode[i].data();
				VkShaderModule shaderModule;
				if (vkCreateShaderModule(m_Device->GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
				{
					GX_CORE_ERROR("Failed to create shader module for: {0}", shaderFilePath.string());
					return;
				}
				m_ShaderModules.push_back(shaderModule);
			}

			m_Reflection = m_SerializedShaderData.Reflection;
		}
	}

	void VulkanMaterial::CreatePipelineLayout()
	{
		auto& layouts = m_Device->GetGlobalDescriptorSetLayouts();

		VkPipelineLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount = layouts.size();
		info.pSetLayouts = layouts.data();

		auto reflectPushConstants = m_Reflection.GetPushConstantRanges();
		std::vector<VkPushConstantRange> pushConstantRanges;
		pushConstantRanges.reserve(reflectPushConstants.size());

		for (uint32_t i = 0; i < reflectPushConstants.size(); i++)
		{
			VkPushConstantRange pushConstantRange{};
			pushConstantRange.offset = reflectPushConstants[i].Offset;
			pushConstantRange.size = reflectPushConstants[i].Size;
			pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL;
			pushConstantRanges.push_back(pushConstantRange);

		}
		m_PushConstantSize = m_Reflection.GetPushConstantSize();
		if (!pushConstantRanges.empty())
		{
			info.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
			info.pPushConstantRanges = pushConstantRanges.data();
		}
		else
		{
			info.pushConstantRangeCount = 0;
			info.pPushConstantRanges = nullptr;
		}

		if (vkCreatePipelineLayout(m_Device->GetDevice(), &info, nullptr, &m_PipelineLayout) != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to create pipeline layout for material: {0}", m_DebugName);
			return;
		}
	}

	void VulkanMaterial::CreateGraphicsPipeline(const MaterialSpecification& spec)
	{
		uint32_t stride;
		std::vector<VkVertexInputAttributeDescription> vertexAttributes = GetVertexAttributes(&stride);

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		for (uint32_t i = 0; i < m_ShaderModules.size(); i++)
		{
			VkPipelineShaderStageCreateInfo stageInfo{};
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.stage = m_Reflection.GetEntryPoints()[i].Stage == ShaderStage::Vertex ? VK_SHADER_STAGE_VERTEX_BIT :
				(m_Reflection.GetEntryPoints()[i].Stage == ShaderStage::Fragment ? VK_SHADER_STAGE_FRAGMENT_BIT : VK_SHADER_STAGE_COMPUTE_BIT);
			stageInfo.module = m_ShaderModules[i];
			stageInfo.pName = m_Reflection.GetEntryPoints()[i].Name.c_str();
			shaderStages.push_back(stageInfo);
		}

		PipelineBuilder builder;
		builder.SetShaders(shaderStages);
		switch (spec.BlendingMode)
		{
		case Blending::Additive:
			builder.EnableBlendingAdditive();
			break;
		case Blending::Alphablend:
			builder.EnableBlendingAlphablend();
			break;
		case Blending::None:
			builder.DisableBlending();
		}

		switch (spec.GraphicsTopology)
		{
		case Topology::TriangleList:
			builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			break;
		case Topology::TriangleStrip:
			builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
			break;
		case Topology::LineList:
			builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
			break;
		case Topology::LineStrip:
			builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
			break;
		case Topology::PointList:
			builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
			break;
		}

		switch (spec.FillMode)
		{
		case Fill::Solid:
			builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
			break;
		case Fill::Wireframe:
			builder.SetPolygonMode(VK_POLYGON_MODE_LINE);
			break;
		}


		VkFrontFace frontFace = spec.FrontFaceWinding == FrontFace::Clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
		switch (spec.CullMode)
		{
		case Cull::Back:
			builder.SetCullMode(VK_CULL_MODE_BACK_BIT, frontFace);
			break;
		case Cull::Front:
			builder.SetCullMode(VK_CULL_MODE_FRONT_BIT, frontFace);
			break;
		case Cull::None:
			builder.SetCullMode(VK_CULL_MODE_NONE, frontFace);
			break;
		}

		if (stride != 0)
			builder.SetVertexInputs(vertexAttributes, stride);

		if (spec.RenderTarget != nullptr)
		{
			VulkanFramebuffer* fb = (VulkanFramebuffer*)spec.RenderTarget.get();
			builder.SetMultiSampling(fb->IsUsingSamples());
			builder.SetColorAttachments(fb->GetColorAttachmentFormats());

			uint32_t depthImageIndex = fb->GetDepthAttachmentIndex();
			if (depthImageIndex != -1)
			{
				builder.SetDepthFormat(fb->GetImage(depthImageIndex).ImageFormat);
			}
		}
		else
		{
			// Drawing directly to swapchain
			builder.SetMultiSampling(false);  // Swapchain typically doesn't use MSAA

			// Get swapchain format for color attachment
			VkFormat swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
			builder.SetColorAttachments({ swapchainFormat });
		}

		auto MapCompareOp = [](CompareOp op) -> VkCompareOp
			{
				switch (op)
				{
				case CompareOp::Never:
					return VK_COMPARE_OP_NEVER;
				case CompareOp::Less:
					return VK_COMPARE_OP_LESS;
				case CompareOp::Equal:
					return VK_COMPARE_OP_EQUAL;
				case CompareOp::LessOrEqual:
					return VK_COMPARE_OP_LESS_OR_EQUAL;
				case CompareOp::Greater:
					return VK_COMPARE_OP_GREATER;
				case CompareOp::NotEqual:
					return VK_COMPARE_OP_NOT_EQUAL;
				case CompareOp::GreaterOrEqual:
					return VK_COMPARE_OP_GREATER_OR_EQUAL;
				case CompareOp::Always:
					return VK_COMPARE_OP_ALWAYS;
				default:
					return VK_COMPARE_OP_NEVER; // or some default
				}
			};

		if (spec.EnableDepthTest)
			builder.EnableDepthTest(spec.EnableDepthWrite, MapCompareOp(spec.DepthCompareOp));
		else
			builder.DisableDepthTest();

		VkPipelineCacheCreateInfo pipelineCacheCreateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
		if (!m_ShouldRegeneratePipelineCache)
		{
			pipelineCacheCreateInfo.initialDataSize = m_SerializedShaderData.PipelineCache.size();
			pipelineCacheCreateInfo.pInitialData = m_SerializedShaderData.PipelineCache.data();

			vkCreatePipelineCache(m_Device->GetDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache);
		}
		else 
		{
			vkCreatePipelineCache(m_Device->GetDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache);

			size_t cacheSize = 0;
			vkGetPipelineCacheData(m_Device->GetDevice(), m_PipelineCache, &cacheSize, nullptr);
			if (cacheSize > 0)
			{
				m_SerializedShaderData.PipelineCache.resize(cacheSize);
				vkGetPipelineCacheData(m_Device->GetDevice(), m_PipelineCache, &cacheSize, m_SerializedShaderData.PipelineCache.data());
			}
		}

		builder.Cache = m_PipelineCache;
		builder.Layout = m_PipelineLayout;
		m_Pipeline = builder.BuildPipeline(m_Device->GetDevice());
	}

	void VulkanMaterial::CreateComputePipeline()
	{
		VkPipelineShaderStageCreateInfo stageInfo{};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stageInfo.module = m_ShaderModules[0];
		stageInfo.pName = m_Reflection.GetEntryPoints()[0].Name.c_str();

		VkPipelineCacheCreateInfo pipelineCacheCreateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
		if (!m_ShouldRegeneratePipelineCache)
		{
			pipelineCacheCreateInfo.initialDataSize = m_SerializedShaderData.PipelineCache.size();
			pipelineCacheCreateInfo.pInitialData = m_SerializedShaderData.PipelineCache.data();

			vkCreatePipelineCache(m_Device->GetDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache);
		}
		else
		{
			vkCreatePipelineCache(m_Device->GetDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache);

			size_t cacheSize = 0;
			vkGetPipelineCacheData(m_Device->GetDevice(), m_PipelineCache, &cacheSize, nullptr);
			if (cacheSize > 0)
			{
				m_SerializedShaderData.PipelineCache.resize(cacheSize);
				vkGetPipelineCacheData(m_Device->GetDevice(), m_PipelineCache, &cacheSize, m_SerializedShaderData.PipelineCache.data());
			}
		}

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage = stageInfo;
		pipelineInfo.layout = m_PipelineLayout;

		if (vkCreateComputePipelines(m_Device->GetDevice(), m_PipelineCache, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to create compute pipeline for material: {0}", m_DebugName);
			return;
		}
	}

	std::vector<VkVertexInputAttributeDescription> VulkanMaterial::GetVertexAttributes(uint32_t* stride)
	{
		std::vector<VkVertexInputAttributeDescription> attributes;

		// Get vertex attributes from the first (vertex) entry point
		const auto& vertexAttributes = m_Reflection.GetVertexAttributes();

		for (const auto& attribute : vertexAttributes)
		{
			VkVertexInputAttributeDescription vkAttribute{};
			vkAttribute.location = attribute.Location;
			vkAttribute.binding = 0; // Assuming single vertex buffer for now
			vkAttribute.format = ShaderDataTypeToVulkanFormat(attribute.Type);
			vkAttribute.offset = attribute.Offset;

			attributes.push_back(vkAttribute);

			GX_CORE_TRACE("Created Vulkan vertex attribute: location={}, format={}, offset={}",
				vkAttribute.location, static_cast<int>(vkAttribute.format), vkAttribute.offset);
		}

		*stride = m_Reflection.GetVertexStride();

		return attributes;
	}

}