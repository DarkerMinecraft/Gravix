#pragma once

#include <vulkan/vulkan.h>
#include "Renderer/Specification.h"
#include "Core/Log.h"

// VK_CHECK macro for Vulkan error checking
#define VK_CHECK(x) \
	do { \
		VkResult err = x; \
		if (err != VK_SUCCESS) { \
			GX_CORE_ERROR("Vulkan Error: {0}", static_cast<int>(err)); \
		} \
	} while (0)

namespace Gravix
{

	class VulkanUtils
	{
	public:
		static void TransitionImage(VkCommandBuffer cmd, VkImage image, VkFormat format, VkImageLayout currentLayout, VkImageLayout newLayout);
		static void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
		static void ResolveImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D size);

		static bool IsDepthFormat(VkFormat format);
		static bool IsStencilFormat(VkFormat format);

		// Conversion functions from engine types to Vulkan types
		static VkPrimitiveTopology ToVkPrimitiveTopology(Topology topology);
		static VkPolygonMode ToVkPolygonMode(Fill fill);
		static VkCullModeFlags ToVkCullMode(Cull cull);
		static VkFrontFace ToVkFrontFace(FrontFace frontFace);
		static VkCompareOp ToVkCompareOp(CompareOp compareOp);
	};

	struct PipelineBuilder
	{
		std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;

		VkPipelineInputAssemblyStateCreateInfo InputAssembly;
		VkPipelineRasterizationStateCreateInfo Rasterizer;
		VkPipelineColorBlendAttachmentState ColorBlendAttachment;
		VkPipelineMultisampleStateCreateInfo Multisampling;
		VkPipelineLayout Layout;
		VkPipelineCache Cache;
		VkPipelineDepthStencilStateCreateInfo DepthStencil;
		VkPipelineRenderingCreateInfo RenderInfo;
		VkPipelineVertexInputStateCreateInfo VertexInputState;
		std::vector<VkFormat> ColorAttachmentFormats;

		std::vector<VkVertexInputAttributeDescription> VertexAttributes;
		VkVertexInputBindingDescription VertexBinding;

		void SetShaders(std::vector<VkPipelineShaderStageCreateInfo> shaderStages) { ShaderStages = shaderStages; }
		void SetVertexInputs(std::vector<VkVertexInputAttributeDescription> vertexAttributes, uint32_t stride);
		void SetInputTopology(VkPrimitiveTopology topology);
		void SetPolygonMode(VkPolygonMode polygonMode);
		void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
		void SetLineWidth(float lineWidth);
		void SetMultiSampling(bool useSampling);
		void DisableBlending();
		void EnableBlendingAdditive();
		void EnableBlendingAlphablend();
		void SetColorAttachments(std::vector<VkFormat> colorAttachments);
		void SetDepthFormat(VkFormat depthFormat);
		void DisableDepthTest();
		void EnableDepthTest(bool depthWriteEnable, VkCompareOp op);

		PipelineBuilder() { Clear(); }
		void Clear();

		VkPipeline BuildPipeline(VkDevice device);
	};

}