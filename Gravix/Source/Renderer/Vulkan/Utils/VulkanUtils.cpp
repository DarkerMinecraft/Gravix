#include "pch.h"
#include "VulkanUtils.h"

#include "VulkanInitializers.h"
#include "Renderer/Vulkan/VulkanRenderCaps.h"

namespace Gravix 
{

	void VulkanUtils::TransitionImage(VkCommandBuffer cmd, VkImage image, VkFormat imageFormat, VkImageLayout currentLayout, VkImageLayout newLayout)
	{
		VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		imageBarrier.pNext = nullptr;

		imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
		imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

		imageBarrier.oldLayout = currentLayout;
		imageBarrier.newLayout = newLayout;

		VkImageAspectFlags aspectMask = 0;

		// Determine aspect mask based on image format
		switch (imageFormat)
		{
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			break;
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D32_SFLOAT:
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			break;
		default:
			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			break;
		}

		imageBarrier.subresourceRange = VulkanInitializers::ImageSubresourceRange(aspectMask);
		imageBarrier.image = image;

		VkDependencyInfo depInfo{};
		depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		depInfo.pNext = nullptr;

		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &imageBarrier;

		vkCmdPipelineBarrier2(cmd, &depInfo);
	}


	void VulkanUtils::CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize)
	{
		VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

		blitRegion.srcOffsets[1].x = srcSize.width;
		blitRegion.srcOffsets[1].y = srcSize.height;
		blitRegion.srcOffsets[1].z = 1;

		blitRegion.dstOffsets[1].x = dstSize.width;
		blitRegion.dstOffsets[1].y = dstSize.height;
		blitRegion.dstOffsets[1].z = 1;

		blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.srcSubresource.baseArrayLayer = 0;
		blitRegion.srcSubresource.layerCount = 1;
		blitRegion.srcSubresource.mipLevel = 0;

		blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.dstSubresource.baseArrayLayer = 0;
		blitRegion.dstSubresource.layerCount = 1;
		blitRegion.dstSubresource.mipLevel = 0;

		VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
		blitInfo.dstImage = destination;
		blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		blitInfo.srcImage = source;
		blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		blitInfo.filter = VK_FILTER_LINEAR;
		blitInfo.regionCount = 1;
		blitInfo.pRegions = &blitRegion;

		vkCmdBlitImage2(cmd, &blitInfo);
	}

	void VulkanUtils::ResolveImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D size)
	{

		VkImageResolve resolveRegion{};
		resolveRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		resolveRegion.srcSubresource.mipLevel = 0;
		resolveRegion.srcSubresource.baseArrayLayer = 0;
		resolveRegion.srcSubresource.layerCount = 1;
		resolveRegion.srcOffset = { 0, 0, 0 };

		resolveRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		resolveRegion.dstSubresource.mipLevel = 0;
		resolveRegion.dstSubresource.baseArrayLayer = 0;
		resolveRegion.dstSubresource.layerCount = 1;
		resolveRegion.dstOffset = { 0, 0, 0 };

		resolveRegion.extent.width = size.width;
		resolveRegion.extent.height = size.height;
		resolveRegion.extent.depth = 1;

		vkCmdResolveImage(
			cmd,
			source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &resolveRegion
		);
	}

	bool VulkanUtils::IsDepthFormat(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_X8_D24_UNORM_PACK32:
		case VK_FORMAT_D32_SFLOAT:
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return true;
		default:
			return false;
		}
	}

	bool VulkanUtils::IsStencilFormat(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_S8_UINT:
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return true;
		default:
			return false;
		}
	}

	void PipelineBuilder::SetVertexInputs(std::vector<VkVertexInputAttributeDescription> vertexAttributes, uint32_t stride)
	{
		VertexAttributes = std::move(vertexAttributes);

		VertexBinding = {};
		VertexBinding.binding = 0;
		VertexBinding.stride = stride;
		VertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VertexInputState.vertexBindingDescriptionCount = stride > 0 ? 1 : 0;
		VertexInputState.pVertexBindingDescriptions = stride > 0 ? &VertexBinding : nullptr;
		VertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(VertexAttributes.size());
		VertexInputState.pVertexAttributeDescriptions = VertexAttributes.data();
	}

	void PipelineBuilder::SetInputTopology(VkPrimitiveTopology topology)
	{
		InputAssembly.topology = topology;
		InputAssembly.primitiveRestartEnable = false;
	}

	void PipelineBuilder::SetPolygonMode(VkPolygonMode polygonMode)
	{
		Rasterizer.polygonMode = polygonMode;
	}

	void PipelineBuilder::SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
	{
		Rasterizer.cullMode = cullMode;
		Rasterizer.frontFace = frontFace;
	}

	void PipelineBuilder::SetLineWidth(float lineWidth)
	{
		Rasterizer.lineWidth = lineWidth;
	}

	void PipelineBuilder::SetMultiSampling(bool useSampling)
	{
		Multisampling.sampleShadingEnable = useSampling;
		// multisampling defaulted to no multisampling (1 sample per pixel)
		Multisampling.rasterizationSamples = useSampling ? VulkanRenderCaps::GetSampleCount() : VK_SAMPLE_COUNT_1_BIT;
		Multisampling.minSampleShading = 1.0f;
		Multisampling.pSampleMask = nullptr;
		// no alpha to coverage either
		Multisampling.alphaToCoverageEnable = VK_FALSE;
		Multisampling.alphaToOneEnable = VK_FALSE;
	}

	void PipelineBuilder::DisableBlending()
	{
		// default write mask
		ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		// no blending
		ColorBlendAttachment.blendEnable = VK_FALSE;
	}

	void PipelineBuilder::EnableBlendingAdditive()
	{
		ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		ColorBlendAttachment.blendEnable = VK_TRUE;
		ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	void PipelineBuilder::EnableBlendingAlphablend()
	{
		ColorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		ColorBlendAttachment.blendEnable = VK_TRUE;
		ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	void PipelineBuilder::SetColorAttachments(std::vector<VkFormat> colorAttachments)
	{
		ColorAttachmentFormats = colorAttachments;

		RenderInfo.colorAttachmentCount = ColorAttachmentFormats.size();
		RenderInfo.pColorAttachmentFormats = ColorAttachmentFormats.data();
	}

	void PipelineBuilder::SetDepthFormat(VkFormat depthFormat)
	{
		RenderInfo.depthAttachmentFormat = depthFormat;
	}

	void PipelineBuilder::DisableDepthTest()
	{
		DepthStencil.depthTestEnable = VK_FALSE;
		DepthStencil.depthWriteEnable = VK_FALSE;
		DepthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
		DepthStencil.depthBoundsTestEnable = VK_FALSE;
		DepthStencil.stencilTestEnable = VK_FALSE;
		DepthStencil.front = {};
		DepthStencil.back = {};
		DepthStencil.minDepthBounds = 0.f;
		DepthStencil.maxDepthBounds = 1.f;
	}

	void PipelineBuilder::EnableDepthTest(bool depthWriteEnable, VkCompareOp op)
	{
		DepthStencil.depthTestEnable = VK_TRUE;
		DepthStencil.depthWriteEnable = depthWriteEnable;
		DepthStencil.depthCompareOp = op;
		DepthStencil.depthBoundsTestEnable = VK_FALSE;
		DepthStencil.stencilTestEnable = VK_FALSE;
		DepthStencil.front = {};
		DepthStencil.back = {};
		DepthStencil.minDepthBounds = 0.f;
		DepthStencil.maxDepthBounds = 1.f;
	}

	void PipelineBuilder::Clear()
	{
		VertexInputState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		InputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		Rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		ColorBlendAttachment = {};
		Multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		Layout = {};
		Cache = {};
		DepthStencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		RenderInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
		ShaderStages.clear();
	}

	VkPipeline PipelineBuilder::BuildPipeline(VkDevice device)
	{
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.pNext = nullptr;

		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// setup dummy color blending. We arent using transparent objects yet
		// the blending is just "no blend", but we do write to the color attachment
		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.pNext = nullptr;

		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		uint32_t attachmentCount = RenderInfo.colorAttachmentCount;

		// create an array of blend attachments (one per color attachment)
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
		blendAttachments.resize(attachmentCount);
		for (uint32_t i = 0; i < attachmentCount; ++i) {
			blendAttachments[i] = ColorBlendAttachment; // copy base settings
		}

		// If some formats do not support blending, ensure blendEnable = VK_FALSE for them.
		// (Quick check by known formats; for robust check use vkGetPhysicalDeviceFormatProperties)
		for (uint32_t i = 0; i < attachmentCount; ++i) {
			if (i >= ColorAttachmentFormats.size()) break; // defensive
			VkFormat fmt = ColorAttachmentFormats[i];

			// Example fast-path: formats without alpha / integer formats generally don't support blending
			// Your actual check can be more thorough by querying physical device format features.
			if (fmt == VK_FORMAT_R8_UINT || fmt == VK_FORMAT_R8_SINT 
				|| fmt == VK_FORMAT_R32_SINT || fmt == VK_FORMAT_R32_UINT
				|| fmt == VK_FORMAT_R32_SFLOAT) {
				blendAttachments[i].blendEnable = VK_FALSE;
			}
		}

		// If you intend to have different blending per attachment and the device didn't enable
		// independentBlend, you must enable independentBlend when creating the device.
		// Otherwise all elements must be identical. The code above copies the same base state
		// and selectively disables blending for formats that don't support it (so they may differ).
		// If you cannot differ, ensure all attachments are identical OR enable independentBlend.
		colorBlending.attachmentCount = attachmentCount;
		colorBlending.pAttachments = blendAttachments.data();

		// build the actual pipeline
		// we now use all of the info structs we have been writing into into this one
		// to create the pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		// connect the renderInfo to the pNext extension mechanism
		pipelineInfo.pNext = &RenderInfo;

		pipelineInfo.stageCount = (uint32_t)ShaderStages.size();
		pipelineInfo.pStages = ShaderStages.data();
		pipelineInfo.pVertexInputState = &VertexInputState;
		pipelineInfo.pInputAssemblyState = &InputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &Rasterizer;
		pipelineInfo.pMultisampleState = &Multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDepthStencilState = &DepthStencil;
		pipelineInfo.layout = Layout;

		VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH };
		VkPipelineDynamicStateCreateInfo dynamicInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		dynamicInfo.pDynamicStates = &state[0];
		dynamicInfo.dynamicStateCount = 3;
		pipelineInfo.pDynamicState = &dynamicInfo;

		VkPipeline pipeline;
		vkCreateGraphicsPipelines(device, Cache, 1, &pipelineInfo, nullptr, &pipeline);
		return pipeline;
	}

	VkPrimitiveTopology VulkanUtils::ToVkPrimitiveTopology(Topology topology)
	{
		switch (topology)
		{
		case Topology::PointList:     return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case Topology::LineList:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case Topology::LineStrip:     return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case Topology::TriangleList:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case Topology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		default:                      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}
	}

	VkPolygonMode VulkanUtils::ToVkPolygonMode(Fill fill)
	{
		switch (fill)
		{
		case Fill::Solid:      return VK_POLYGON_MODE_FILL;
		case Fill::Wireframe:  return VK_POLYGON_MODE_LINE;
		case Fill::Point:      return VK_POLYGON_MODE_POINT;
		default:               return VK_POLYGON_MODE_FILL;
		}
	}

	VkCullModeFlags VulkanUtils::ToVkCullMode(Cull cull)
	{
		switch (cull)
		{
		case Cull::None:       return VK_CULL_MODE_NONE;
		case Cull::Front:      return VK_CULL_MODE_FRONT_BIT;
		case Cull::Back:       return VK_CULL_MODE_BACK_BIT;
		case Cull::FrontBack:  return VK_CULL_MODE_FRONT_AND_BACK;
		default:               return VK_CULL_MODE_NONE;
		}
	}

	VkFrontFace VulkanUtils::ToVkFrontFace(FrontFace frontFace)
	{
		switch (frontFace)
		{
		case FrontFace::Clockwise:        return VK_FRONT_FACE_CLOCKWISE;
		case FrontFace::CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		default:                          return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		}
	}

	VkCompareOp VulkanUtils::ToVkCompareOp(CompareOp compareOp)
	{
		switch (compareOp)
		{
		case CompareOp::Never:          return VK_COMPARE_OP_NEVER;
		case CompareOp::Less:           return VK_COMPARE_OP_LESS;
		case CompareOp::Equal:          return VK_COMPARE_OP_EQUAL;
		case CompareOp::LessOrEqual:    return VK_COMPARE_OP_LESS_OR_EQUAL;
		case CompareOp::Greater:        return VK_COMPARE_OP_GREATER;
		case CompareOp::NotEqual:       return VK_COMPARE_OP_NOT_EQUAL;
		case CompareOp::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case CompareOp::Always:         return VK_COMPARE_OP_ALWAYS;
		default:                        return VK_COMPARE_OP_LESS;
		}
	}

}