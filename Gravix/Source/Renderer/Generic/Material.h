#pragma once

#include "Renderer/Specification.h"
#include "Renderer/Generic/Framebuffer.h"

#include <string>
#include <filesystem>

namespace Gravix 
{

	struct MaterialSpecification 
	{
		const std::string DebugName; 
		const std::filesystem::path ShaderFilePath;

		Blending BlendingMode;

		bool EnableDepthTest = false;
		bool EnableCulling = false;
		Cull CullMode = Cull::Back;
		Fill FillMode = Fill::Solid;

		Topology GraphicsTopology = Topology::TriangleList;

		float LineWidth = 1.0f;
		size_t PushConstantSize = 0;

		Ref<Framebuffer> RenderTarget;
	};

	class Material 
	{
	public:
		virtual ~Material() = default;

		static Ref<Material> Create(const MaterialSpecification& spec);
		static Ref<Material> Create(const std::string& debugName, const std::filesystem::path& shaderFilePath);
	};

}