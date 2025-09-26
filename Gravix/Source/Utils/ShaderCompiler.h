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
		ShaderStage SlangStageToShaderStage(SlangStage stage);
		void ExtractComputeDispatchInfo(slang::EntryPointReflection* entryPoint, ShaderReflection* reflection);
		void ExtractBuffers(ShaderStage stage, slang::ProgramLayout* layout, slang::IMetadata* metadata, ShaderReflection* reflection);
		void ExtractVertexAttributes(ShaderStage stage, slang::EntryPointReflection* entryPointReflection, ShaderReflection* reflection);
		PushConstant ExtractPushConstant(slang::VariableLayoutReflection* field, ShaderStage stage);

		ShaderDataType SlangTypeToShaderDataType(slang::TypeReflection* type);
		std::string GenerateSemanticFromName(const std::string& name);
		const char* ShaderDataTypeToString(ShaderDataType type);
	private:
		Slang::ComPtr<slang::IGlobalSession> m_GlobalSession;
	};

}