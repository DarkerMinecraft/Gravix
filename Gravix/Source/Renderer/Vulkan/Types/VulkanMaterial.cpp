#include "pch.h"
#include "VulkanMaterial.h"
#include "VulkanFramebuffer.h"
#include "VulkanTexture.h"

#include "Renderer/Vulkan/Utils/DescriptorWriter.h"
#include "Renderer/Vulkan/Utils/VulkanUtils.h"
#include "Asset/EditorAssetManager.h"
#include "Project/Project.h"

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

	VulkanMaterial::VulkanMaterial(Device* device, AssetHandle shaderHandle, AssetHandle pipelineHandle)
		: m_Device(static_cast<VulkanDevice*>(device))
	{
		// Handle null handles (for materials created from editor without assigned shader/pipeline)
		if (shaderHandle == 0 || pipelineHandle == 0)
		{
			GX_CORE_WARN("Created material with null shader or pipeline handle. Assign them before rendering.");
			return;
		}

		// Load Shader and Pipeline from AssetManager
		auto assetManager = Project::GetActive()->GetEditorAssetManager();

		m_Shader = assetManager->GetAsset(shaderHandle);
		m_Pipeline = assetManager->GetAsset(pipelineHandle);

		if (!m_Shader)
		{
			GX_CORE_ERROR("Failed to load shader for material!");
			return;
		}

		if (!m_Pipeline)
		{
			GX_CORE_ERROR("Failed to load pipeline for material!");
			return;
		}

		// Determine if this is a compute shader
		m_IsCompute = (m_Shader->GetShaderType() == ShaderType::Compute);

		// Create shader modules from SPIR-V
		CreateShaderModules();

		GX_CORE_INFO("Created VulkanMaterial (pipeline not yet built)");
	}

	VulkanMaterial::VulkanMaterial(Device* device, Ref<Shader> shader, Ref<Pipeline> pipeline)
		: m_Device(static_cast<VulkanDevice*>(device)), m_Shader(shader), m_Pipeline(pipeline)
	{
		if (!m_Shader)
		{
			GX_CORE_WARN("Created material with null shader. Assign shader before rendering.");
			return;
		}

		if (!m_Pipeline)
		{
			GX_CORE_WARN("Created material with null pipeline. Assign pipeline before rendering.");
			return;
		}

		// Determine if this is a compute shader
		m_IsCompute = (m_Shader->GetShaderType() == ShaderType::Compute);

		// Create shader modules from SPIR-V
		CreateShaderModules();

		GX_CORE_INFO("Created VulkanMaterial from direct references (pipeline not yet built)");
	}

	void VulkanMaterial::SetFramebuffer(Ref<Framebuffer> framebuffer)
	{
		m_RenderTarget = framebuffer;
		BuildPipeline();
	}

	void VulkanMaterial::BuildPipeline()
	{
		if (m_PipelineBuilt)
		{
			// Destroy old pipeline
			if (m_VkPipeline != VK_NULL_HANDLE)
				vkDestroyPipeline(m_Device->GetDevice(), m_VkPipeline, nullptr);
			if (m_PipelineLayout != VK_NULL_HANDLE)
				vkDestroyPipelineLayout(m_Device->GetDevice(), m_PipelineLayout, nullptr);
		}

		// Create pipeline layout
		CreatePipelineLayout();

		// Create graphics or compute pipeline
		if (m_IsCompute)
		{
			CreateComputePipeline();
		}
		else
		{
			CreateGraphicsPipeline();
		}

		m_PipelineBuilt = true;
		GX_CORE_INFO("Built Vulkan pipeline for material");
	}

	VulkanMaterial::~VulkanMaterial()
	{
		for (auto& shaderModule : m_ShaderModules)
		{
			vkDestroyShaderModule(m_Device->GetDevice(), shaderModule, nullptr);
		}

		if (m_PipelineLayout != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(m_Device->GetDevice(), m_PipelineLayout, nullptr);

		if (m_VkPipeline != VK_NULL_HANDLE)
			vkDestroyPipeline(m_Device->GetDevice(), m_VkPipeline, nullptr);
	}

	DynamicStruct VulkanMaterial::GetPushConstantStruct()
	{
		return DynamicStruct(m_Shader->GetReflection().GetReflectedStruct("PushConstants"));
	}

	DynamicStruct VulkanMaterial::GetMaterialStruct()
	{
		return DynamicStruct(m_Shader->GetReflection().GetReflectedStruct("Material"));
	}

	DynamicStruct VulkanMaterial::GetVertexStruct()
	{
		return DynamicStruct(m_Shader->GetReflection().GetReflectedStruct("Vertex"));
	}

	size_t VulkanMaterial::GetVertexSize()
	{
		return m_Shader->GetReflection().GetReflectedStruct("Vertex").GetSize();
	}

	ReflectedStruct VulkanMaterial::GetReflectedStruct(const std::string& name)
	{
		return m_Shader->GetReflection().GetReflectedStruct(name);
	}

	void VulkanMaterial::Bind(VkCommandBuffer cmd, void* pushConstants)
	{
		VkPipelineBindPoint bindPoint = m_IsCompute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;

		vkCmdBindPipeline(cmd, bindPoint, m_VkPipeline);

		// Bind all bindless descriptor sets (set 0: storage buffers, set 1: textures, set 2: storage images)
		VkDescriptorSet* descriptorSets = m_Device->GetGlobalDescriptorSets();
		uint32_t setCount = static_cast<uint32_t>(m_Device->GetGlobalDescriptorSetLayouts().size());
		vkCmdBindDescriptorSets(cmd, bindPoint, m_PipelineLayout, 0, setCount, descriptorSets, 0, nullptr);

		if (pushConstants && m_PushConstantSize > 0)
		{
			VkShaderStageFlags stages = m_IsCompute ? VK_SHADER_STAGE_COMPUTE_BIT : (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
			vkCmdPushConstants(cmd, m_PipelineLayout, stages, 0, m_PushConstantSize, pushConstants);
		}
	}

	void VulkanMaterial::Dispatch(VkCommandBuffer cmd, uint32_t width, uint32_t height)
	{
		if (!m_IsCompute)
		{
			GX_CORE_ERROR("Dispatch called on non-compute material!");
			return;
		}

		auto& computeInfo = m_Shader->GetReflection().GetComputeDispatch();
		uint32_t groupsX = (width + computeInfo.LocalSizeX - 1) / computeInfo.LocalSizeX;
		uint32_t groupsY = (height + computeInfo.LocalSizeY - 1) / computeInfo.LocalSizeY;
		uint32_t groupsZ = 1;

		vkCmdDispatch(cmd, groupsX, groupsY, groupsZ);
	}

	void VulkanMaterial::BindResource(VkCommandBuffer cmd, uint32_t binding, Framebuffer* buffer, uint32_t index, bool sampler)
	{
		VulkanFramebuffer* vulkanFramebuffer = static_cast<VulkanFramebuffer*>(buffer);
		DescriptorWriter writer;

		if (sampler)
		{
			// Combined image samplers use set 1
			writer.WriteImage(binding, vulkanFramebuffer->GetAttachmentImageView(index),
				m_Device->GetLinearSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.UpdateSet(m_Device->GetDevice(), m_Device->GetGlobalDescriptorSet(1));
		}
		else
		{
			// Storage images use set 2
			writer.WriteImage(binding, vulkanFramebuffer->GetAttachmentImageView(index),
				VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
			writer.UpdateSet(m_Device->GetDevice(), m_Device->GetGlobalDescriptorSet(2));
		}
	}

	void VulkanMaterial::BindResource(VkCommandBuffer cmd, uint32_t binding, Texture2D* texture)
	{
		VulkanTexture2D* vulkanTexture = static_cast<VulkanTexture2D*>(texture);
		DescriptorWriter writer;

		// Combined image samplers use set 1
		writer.WriteImage(binding, vulkanTexture->GetImageView(),
			vulkanTexture->GetVkSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		writer.UpdateSet(m_Device->GetDevice(), m_Device->GetGlobalDescriptorSet(1));
	}

	void VulkanMaterial::BindResource(VkCommandBuffer cmd, uint32_t binding, uint32_t index, Texture2D* texture)
	{
		VulkanTexture2D* vulkanTexture = static_cast<VulkanTexture2D*>(texture);
		DescriptorWriter writer;

		// Combined image samplers use set 1
		writer.WriteImage(binding, vulkanTexture->GetImageView(),
			vulkanTexture->GetVkSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, index);

		writer.UpdateSet(m_Device->GetDevice(), m_Device->GetGlobalDescriptorSet(1));
	}

	void VulkanMaterial::CreateShaderModules()
	{
		const auto& spirvCode = m_Shader->GetSPIRV();

		for (const auto& spirv : spirvCode)
		{
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = spirv.size() * sizeof(uint32_t);
			createInfo.pCode = spirv.data();

			VkShaderModule shaderModule;
			VK_CHECK(vkCreateShaderModule(m_Device->GetDevice(), &createInfo, nullptr, &shaderModule));

			m_ShaderModules.push_back(shaderModule);
		}
	}

	void VulkanMaterial::CreatePipelineLayout()
	{
		// Get push constant size from reflection
		m_PushConstantSize = m_Shader->GetReflection().GetPushConstantSize();

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.offset = 0;
		pushConstantRange.size = m_PushConstantSize;
		pushConstantRange.stageFlags = m_IsCompute ? VK_SHADER_STAGE_COMPUTE_BIT : (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

		// Use all bindless descriptor set layouts (storage buffers, combined image samplers, storage images)
		auto& globalSetLayouts = m_Device->GetGlobalDescriptorSetLayouts();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(globalSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = globalSetLayouts.data();

		if (m_PushConstantSize > 0)
		{
			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		}

		VK_CHECK(vkCreatePipelineLayout(m_Device->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout));
	}

	void VulkanMaterial::CreateGraphicsPipeline()
	{
		const auto& entryPoints = m_Shader->GetReflection().GetEntryPoints();
		const auto& config = m_Pipeline->GetConfiguration();

		// Create shader stage infos
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

		for (size_t i = 0; i < entryPoints.size() && i < m_ShaderModules.size(); i++)
		{
			VkPipelineShaderStageCreateInfo shaderStage{};
			shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStage.stage = ShaderStageToVulkanShaderStage(entryPoints[i].Stage);
			shaderStage.module = m_ShaderModules[i];
			shaderStage.pName = entryPoints[i].Name.c_str();
			shaderStages.push_back(shaderStage);
		}

		// Vertex input
		uint32_t stride = 0;
		auto vertexAttributes = GetVertexAttributes(&stride);

		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = stride;
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size());
		vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();

		// Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VulkanUtils::ToVkPrimitiveTopology(config.GraphicsTopology);
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// Viewport and scissor (dynamic)
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// Rasterization
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VulkanUtils::ToVkPolygonMode(config.FillMode);
		rasterizer.lineWidth = config.LineWidth;
		rasterizer.cullMode = VulkanUtils::ToVkCullMode(config.CullMode);
		rasterizer.frontFace = VulkanUtils::ToVkFrontFace(config.FrontFaceWinding);
		rasterizer.depthBiasEnable = VK_FALSE;

		// Multisampling
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = m_RenderTarget ? static_cast<VulkanFramebuffer*>(m_RenderTarget.get())->GetSampleCount() : VK_SAMPLE_COUNT_1_BIT;

		// Depth stencil
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = config.EnableDepthTest ? VK_TRUE : VK_FALSE;
		depthStencil.depthWriteEnable = config.EnableDepthWrite ? VK_TRUE : VK_FALSE;
		depthStencil.depthCompareOp = VulkanUtils::ToVkCompareOp(config.DepthCompareOp);
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		// Color blending - need one blend state per color attachment
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = (config.BlendingMode != Blending::None) ? VK_TRUE : VK_FALSE;

		if (config.BlendingMode == Blending::Alpha)
		{
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		}
		else if (config.BlendingMode == Blending::Additive)
		{
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		}

		// Create color blend attachments for all color attachments (will be calculated later from framebuffer)
		uint32_t colorAttachmentCount = m_RenderTarget ? static_cast<VulkanFramebuffer*>(m_RenderTarget.get())->GetColorAttachmentFormats().size() : 1;
		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(colorAttachmentCount, colorBlendAttachment);

		// Disable blending for integer formats (like entity ID attachment)
		if (m_RenderTarget)
		{
			const auto& formats = static_cast<VulkanFramebuffer*>(m_RenderTarget.get())->GetColorAttachmentFormats();
			for (size_t i = 0; i < formats.size(); ++i)
			{
				VkFormat fmt = formats[i];
				// Integer formats do not support blending
				if (fmt == VK_FORMAT_R8_UINT || fmt == VK_FORMAT_R8_SINT ||
					fmt == VK_FORMAT_R16_UINT || fmt == VK_FORMAT_R16_SINT ||
					fmt == VK_FORMAT_R32_UINT || fmt == VK_FORMAT_R32_SINT ||
					fmt == VK_FORMAT_R8G8B8A8_UINT || fmt == VK_FORMAT_R8G8B8A8_SINT ||
					fmt == VK_FORMAT_R16G16B16A16_UINT || fmt == VK_FORMAT_R16G16B16A16_SINT ||
					fmt == VK_FORMAT_R32G32B32A32_UINT || fmt == VK_FORMAT_R32G32B32A32_SINT)
				{
					colorBlendAttachments[i].blendEnable = VK_FALSE;
				}
			}
		}

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
		colorBlending.pAttachments = colorBlendAttachments.data();

		// Dynamic state
		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		// Rendering info (dynamic rendering)
		std::vector<VkFormat> colorFormats;
		if (m_RenderTarget)
		{
			colorFormats = static_cast<VulkanFramebuffer*>(m_RenderTarget.get())->GetColorAttachmentFormats();
		}
		else
		{
			colorFormats = { VK_FORMAT_R8G8B8A8_UNORM };
		}
		VkFormat depthFormat = m_RenderTarget ? static_cast<VulkanFramebuffer*>(m_RenderTarget.get())->GetDepthFormat() : VK_FORMAT_D32_SFLOAT;

		VkPipelineRenderingCreateInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
		renderingInfo.pColorAttachmentFormats = colorFormats.data();
		renderingInfo.depthAttachmentFormat = depthFormat;

		// Create graphics pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = &renderingInfo;
		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = m_PipelineLayout;
		pipelineInfo.renderPass = VK_NULL_HANDLE;
		pipelineInfo.subpass = 0;

		VK_CHECK(vkCreateGraphicsPipelines(m_Device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VkPipeline));
	}

	void VulkanMaterial::CreateComputePipeline()
	{
		const auto& entryPoints = m_Shader->GetReflection().GetEntryPoints();

		VkPipelineShaderStageCreateInfo shaderStage{};
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStage.module = m_ShaderModules[0];
		shaderStage.pName = entryPoints[0].Name.c_str();

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage = shaderStage;
		pipelineInfo.layout = m_PipelineLayout;

		VK_CHECK(vkCreateComputePipelines(m_Device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VkPipeline));
	}

	std::vector<VkVertexInputAttributeDescription> VulkanMaterial::GetVertexAttributes(uint32_t* stride)
	{
		const auto& vertexAttributes = m_Shader->GetReflection().GetVertexAttributes();
		std::vector<VkVertexInputAttributeDescription> attributes;

		for (const auto& attrib : vertexAttributes)
		{
			VkVertexInputAttributeDescription attribute{};
			attribute.binding = 0;
			attribute.location = attrib.Location;
			attribute.format = ShaderDataTypeToVulkanFormat(attrib.Type);
			attribute.offset = attrib.Offset;
			attributes.push_back(attribute);
		}

		*stride = m_Shader->GetReflection().GetVertexStride();
		return attributes;
	}

}
