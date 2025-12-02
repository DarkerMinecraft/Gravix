#pragma once

#include "Core/RefCounted.h"
#include "Asset/Asset.h"
#include "Renderer/Generic/Types/Shader.h"
#include "Renderer/Generic/Types/Pipeline.h"
#include "Renderer/Generic/Types/Framebuffer.h"

#include "Reflections/DynamicStruct.h"

#include <string>

namespace Gravix
{

	// Material Asset - combines Shader + Pipeline configuration
	// Stored as .orbmat YAML files
	// Pipeline is built when SetFramebuffer() is called
	class Material : public Asset
	{
	public:
		virtual ~Material() = default;

		virtual AssetType GetAssetType() const override { return AssetType::Material; }

		virtual DynamicStruct GetPushConstantStruct() = 0;
		virtual DynamicStruct GetMaterialStruct() = 0;

		virtual DynamicStruct GetVertexStruct() = 0;
		virtual size_t GetVertexSize() = 0;

		virtual ReflectedStruct GetReflectedStruct(const std::string& name) = 0;

		virtual Ref<Shader> GetShader() const = 0;
		virtual Ref<Pipeline> GetPipeline() const = 0;

		// Set framebuffer and build the Vulkan pipeline
		// Must be called before using the material for rendering
		virtual void SetFramebuffer(Ref<Framebuffer> framebuffer) = 0;
		virtual bool IsReady() const = 0;

		// Create material from Shader and Pipeline assets
		// Pipeline will be built when SetFramebuffer() is called
		static Ref<Material> Create(AssetHandle shaderHandle, AssetHandle pipelineHandle);

		// Create material directly from Shader and Pipeline references (for runtime-created materials)
		static Ref<Material> Create(Ref<Shader> shader, Ref<Pipeline> pipeline);
	};

}