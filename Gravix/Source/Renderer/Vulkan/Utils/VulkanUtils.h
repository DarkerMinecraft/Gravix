#pragma once

#include <vulkan/vulkan.h>

namespace Gravix 
{

	class VulkanUtils 
	{
	public:
		static void TransitionImage(VkCommandBuffer cmd, VkImage image, VkFormat format, VkImageLayout currentLayout, VkImageLayout newLayout);
		static void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
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