#pragma once

#include "Renderer/Specification.h"

#include <slang.h>
#include <string>

namespace Gravix
{

	namespace SlangTypeUtils
	{
		// String conversion utilities
		const char* ShaderStageToString(ShaderStage stage);
		const char* DescriptorTypeToString(DescriptorType type);
		const char* ShaderDataTypeToString(ShaderDataType type);

		// Type conversion from Slang to Gravix types
		ShaderStage SlangStageToShaderStage(SlangStage stage);
		ShaderDataType SlangTypeToShaderDataType(slang::TypeReflection* type);

		// Semantic generation
		std::string GenerateSemanticFromName(const std::string& name);

		// Type size calculation
		uint32_t CalculateTypeSize(slang::TypeReflection* type);
		uint32_t GetCorrectParameterSize(slang::VariableLayoutReflection* varLayout);
	}

}
