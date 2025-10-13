#pragma once

#include "Reflections/ShaderReflection.h"

#include "Renderer/Generic/Material.h"
#include "Renderer/Generic/Texture.h"

#include "VulkanDevice.h"

namespace Gravix 
{

	class VulkanMaterial : public Material 
	{
	public:
		VulkanMaterial(Device* device, const MaterialSpecification& spec);
		VulkanMaterial(Device* device, const std::string& debugName, const std::filesystem::path& shaderFilePath);
		virtual ~VulkanMaterial();

		virtual DynamicStruct GetPushConstantStruct() override { return DynamicStruct(m_Reflection.GetReflectedStruct("PushConstants")); }
		virtual DynamicStruct GetMaterialStruct() override { return DynamicStruct(m_Reflection.GetReflectedStruct("Material")); }
		virtual DynamicStruct GetVertexStruct() override { return DynamicStruct(m_Reflection.GetReflectedStruct("Vertex")); }

		void Bind(VkCommandBuffer cmd, void* pushConstants);
		void Dispatch(VkCommandBuffer cmd, uint32_t width, uint32_t height);

		void BindResource(VkCommandBuffer cmd, uint32_t binding, Framebuffer* buffer, uint32_t index, bool sampler);
		void BindResource(VkCommandBuffer cmd, uint32_t binding, Texture2D* texture);
	private:
		void CreateMaterial(const MaterialSpecification& spec);
		void CreateMaterial(const std::string& debugName, const std::filesystem::path& shaderFilePath);

		void SpinShader(const std::filesystem::path& shaderFilePath);
		void CreatePipelineLayout();

		void CreateGraphicsPipeline(const MaterialSpecification& spec);
		void CreateComputePipeline();

		std::vector<VkVertexInputAttributeDescription> GetVertexAttributes(uint32_t* stride);
	private:
		VulkanDevice* m_Device;
		std::string m_DebugName;

		VkPipeline m_Pipeline;
		VkPipelineLayout m_PipelineLayout;

		std::vector<VkShaderModule> m_ShaderModules;

		ShaderReflection m_Reflection;
		bool m_IsCompute = false;
		uint32_t m_PushConstantSize = 0;
	};

}