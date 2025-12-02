#pragma once

#include "Renderer/Generic/Types/Material.h"
#include "Renderer/Generic/Types/Shader.h"
#include "Renderer/Generic/Types/Pipeline.h"
#include "Renderer/Generic/Types/Texture.h"
#include "Renderer/Generic/Types/Framebuffer.h"

#include "Renderer/Vulkan/VulkanDevice.h"

namespace Gravix
{

	class VulkanMaterial : public Material
	{
	public:
		VulkanMaterial(Device* device, AssetHandle shaderHandle, AssetHandle pipelineHandle);
		VulkanMaterial(Device* device, Ref<Shader> shader, Ref<Pipeline> pipeline);
		virtual ~VulkanMaterial();

		virtual DynamicStruct GetPushConstantStruct() override;
		virtual DynamicStruct GetMaterialStruct() override;
		virtual DynamicStruct GetVertexStruct() override;

		virtual size_t GetVertexSize() override;
		virtual ReflectedStruct GetReflectedStruct(const std::string& name) override;

		virtual Ref<Shader> GetShader() const override { return m_Shader; }
		virtual Ref<Pipeline> GetPipeline() const override { return m_Pipeline; }

		virtual void SetFramebuffer(Ref<Framebuffer> framebuffer) override;
		virtual bool IsReady() const override { return m_PipelineBuilt; }

		void Bind(VkCommandBuffer cmd, void* pushConstants);
		void Dispatch(VkCommandBuffer cmd, uint32_t width, uint32_t height);

		void BindResource(VkCommandBuffer cmd, uint32_t binding, Framebuffer* buffer, uint32_t index, bool sampler);
		void BindResource(VkCommandBuffer cmd, uint32_t binding, Texture2D* texture);
		void BindResource(VkCommandBuffer cmd, uint32_t binding, uint32_t index, Texture2D* texture);

	private:
		void BuildPipeline();
		void CreatePipelineLayout();
		void CreateGraphicsPipeline();
		void CreateComputePipeline();

		void CreateShaderModules();
		std::vector<VkVertexInputAttributeDescription> GetVertexAttributes(uint32_t* stride);

	private:
		VulkanDevice* m_Device;

		Ref<Shader> m_Shader;
		Ref<Pipeline> m_Pipeline;
		Ref<Framebuffer> m_RenderTarget;

		VkPipeline m_VkPipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

		std::vector<VkShaderModule> m_ShaderModules;

		bool m_IsCompute = false;
		bool m_PipelineBuilt = false;
		uint32_t m_PushConstantSize = 0;
	};

}
