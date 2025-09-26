#include "pch.h"
#include "VulkanMaterial.h"
#include "VulkanFramebuffer.h"

#include "Utils/DescriptorWriter.h"

namespace Gravix
{

	VulkanMaterial::VulkanMaterial(Device* device, const MaterialSpecification& spec)
		: m_Device(static_cast<VulkanDevice*>(device)), m_DebugName(spec.DebugName)
	{
		CreateMaterial(spec);
	}

	VulkanMaterial::VulkanMaterial(Device* device, const std::string& debugName, const std::filesystem::path& shaderFilePath)
		: m_Device(static_cast<VulkanDevice*>(device)), m_DebugName(debugName)
	{
		CreateMaterial(debugName, shaderFilePath);
		m_IsCompute = true;
	}

	VulkanMaterial::~VulkanMaterial()
	{
		for (auto& shaderModule : m_ShaderModules)
		{
			vkDestroyShaderModule(m_Device->GetDevice(), shaderModule, nullptr);
		}

		vkDestroyPipelineLayout(m_Device->GetDevice(), m_PipelineLayout, nullptr);
		vkDestroyPipeline(m_Device->GetDevice(), m_Pipeline, nullptr);
	}

	void VulkanMaterial::Bind(VkCommandBuffer cmd, void* pushConstants)
	{
		if(m_Pipeline == VK_NULL_HANDLE)
			return;

		vkCmdBindPipeline(cmd, m_IsCompute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
		vkCmdBindDescriptorSets(cmd, m_IsCompute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
			0, static_cast<uint32_t>(m_Device->GetGlobalDescriptorSetLayouts().size()), m_Device->GetGlobalDescriptorSets(), 0, nullptr);
		if (m_PushConstantSize > 0 && pushConstants != nullptr)
			vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_ALL, 0, m_PushConstantSize, pushConstants);
	}

	void VulkanMaterial::Dispatch(VkCommandBuffer cmd, uint32_t width, uint32_t height)
	{
		if(m_Pipeline == VK_NULL_HANDLE || !m_IsCompute)
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

	void VulkanMaterial::CreateMaterial(const MaterialSpecification& spec)
	{
		SpinShader(spec.ShaderFilePath);
		CreatePipelineLayout();
	}

	void VulkanMaterial::CreateMaterial(const std::string& debugName, const std::filesystem::path& shaderFilePath)
	{
		SpinShader(shaderFilePath);
		CreatePipelineLayout();
		CreateComputePipeline();
	}

	void VulkanMaterial::SpinShader(const std::filesystem::path& shaderFilePath)
	{
		std::vector<std::vector<uint32_t>> spirv;
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
		}
		else
		{
			GX_CORE_ERROR("Failed to compile shader: {0}", shaderFilePath.string());
		}
	}

	void VulkanMaterial::CreatePipelineLayout()
	{
		auto& layouts = m_Device->GetGlobalDescriptorSetLayouts();

		VkPipelineLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount = layouts.size();
		info.pSetLayouts = layouts.data();

		if (m_Reflection.GetPushConstant().Size > 0) 
		{
			VkPushConstantRange pushConstantRange{};
			pushConstantRange.offset = m_Reflection.GetPushConstant().Offset;
			pushConstantRange.size = m_Reflection.GetPushConstant().Size;
			pushConstantRange.stageFlags = m_Reflection.GetPushConstant().Stage == ShaderStage::Vertex ? VK_SHADER_STAGE_VERTEX_BIT :
				(m_Reflection.GetPushConstant().Stage == ShaderStage::Fragment ? VK_SHADER_STAGE_FRAGMENT_BIT : VK_SHADER_STAGE_COMPUTE_BIT);
			info.pushConstantRangeCount = 1;
			info.pPushConstantRanges = &pushConstantRange;

			m_PushConstantSize = m_Reflection.GetPushConstant().Size;
		}

		if (vkCreatePipelineLayout(m_Device->GetDevice(), &info, nullptr, &m_PipelineLayout) != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to create pipeline layout for material: {0}", m_DebugName);
			return;
		}
	}

	void VulkanMaterial::CreateComputePipeline()
	{
		VkPipelineShaderStageCreateInfo stageInfo{};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stageInfo.module = m_ShaderModules[0];
		stageInfo.pName = m_Reflection.GetEntryPoints()[0].Name.c_str();

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage = stageInfo;
		pipelineInfo.layout = m_PipelineLayout;

		if (vkCreateComputePipelines(m_Device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
		{
			GX_CORE_ERROR("Failed to create compute pipeline for material: {0}", m_DebugName);
			return;
		}
	}

}