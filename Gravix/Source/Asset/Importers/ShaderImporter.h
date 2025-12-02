#pragma once

#include "Asset/Asset.h"
#include "Asset/AssetMetadata.h"
#include "Renderer/Generic/Types/Shader.h"

namespace Gravix
{

	class ShaderImporter
	{
	public:
		// Import shader from Slang source file (compiles using global ShaderCompiler)
		// Editor only - uses ShaderCompiler to compile from source
		static Ref<Shader> ImportShader(AssetHandle handle, const AssetMetadata& metadata);

		// Load shader directly from file without asset metadata (for runtime/default materials)
		// Automatically detects shader type if not specified
		static Ref<Shader> LoadFromFile(const std::filesystem::path& shaderPath, ShaderType type = ShaderType::Graphics);

		// Determine shader type from file or metadata
		static ShaderType DetectShaderType(const std::filesystem::path& shaderPath);
	};

}
