#pragma once

#include "Renderer/Specification.h"
#include "Reflections/ShaderReflection.h"

#include <slang.h>
#include <slang-com-ptr.h>

#include <string>
#include <vector>
#include <filesystem>

namespace Gravix
{

	class ShaderCompiler
	{
	public:
		ShaderCompiler();

		bool CompileShader(const std::filesystem::path& filePath, std::vector<std::vector<uint32_t>>* spirvCode, ShaderReflection* reflection);
	private:
		void ExtractPushConstants(slang::ProgramLayout* programLayout, ShaderReflection* reflection);
		void ExtractVertexAttributes(slang::EntryPointReflection* entryPointReflection, ShaderReflection* reflection);
		void ExtractComputeDispatchInfo(slang::EntryPointReflection* entryPoint, ShaderReflection* reflection);

		void ExtractStructs(slang::ProgramLayout* layout, ShaderReflection* reflection);
		void ProcessStructType(slang::TypeLayoutReflection* typeLayout,
			slang::TypeReflection* type,
			std::set<std::string>& processedStructs,
			ShaderReflection* reflection);

		ShaderStage SlangStageToShaderStage(SlangStage stage);
		ShaderDataType SlangTypeToShaderDataType(slang::TypeReflection* type);
		std::string GenerateSemanticFromName(const std::string& name);
		const char* ShaderDataTypeToString(ShaderDataType type);

		uint32_t GetCorrectParameterSize(slang::VariableLayoutReflection* varLayout);
		uint32_t CalculateTypeSize(slang::TypeReflection* type);
	private:
		Slang::ComPtr<slang::IGlobalSession> m_GlobalSession;
	};

}