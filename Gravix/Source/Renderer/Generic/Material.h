#pragma once

#include "Renderer/Specification.h"
#include "Renderer/Generic/Framebuffer.h"

#include "Reflections/DynamicStruct.h"

#include <string>
#include <filesystem>

namespace Gravix 
{

	struct MaterialSpecification 
	{
		std::string DebugName;
		std::filesystem::path ShaderFilePath;

		Blending BlendingMode = Blending::None;

		bool EnableDepthTest = false;
		bool EnableDepthWrite = false;
		CompareOp DepthCompareOp = CompareOp::Less;

		Cull CullMode = Cull::None;
		FrontFace FrontFaceWinding = FrontFace::CounterClockwise;
		Fill FillMode = Fill::Solid;

		Topology GraphicsTopology = Topology::TriangleList;

		float LineWidth = 1.0f;

		Ref<Framebuffer> RenderTarget;
	};

	class Material 
	{
	public:
		virtual ~Material() = default;

		virtual DynamicStruct GetPushConstantStruct() = 0;
		virtual DynamicStruct GetMaterialStruct() = 0;

		virtual DynamicStruct GetVertexStruct() = 0;
		virtual size_t GetVertexSize() = 0;

		virtual ReflectedStruct GetReflectedStruct(const std::string& name) = 0;

		static Ref<Material> Create(const MaterialSpecification& spec);
		static Ref<Material> Create(const std::string& debugName, const std::filesystem::path& shaderFilePath);
	};

}