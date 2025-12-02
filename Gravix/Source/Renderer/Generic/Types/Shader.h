#pragma once

#include "Asset/Asset.h"
#include "Reflections/ShaderReflection.h"

#include <vector>
#include <filesystem>

namespace Gravix
{

	enum class ShaderType
	{
		Graphics,  // Vertex + Fragment shader
		Compute    // Compute shader
	};

	// Platform-agnostic Shader asset
	// Contains compiled SPIR-V bytecode and reflection data
	// Supports multiple entry points from Slang
	class Shader : public Asset
	{
	public:
		virtual ~Shader() = default;

		virtual AssetType GetAssetType() const override { return AssetType::Shader; }

		virtual ShaderType GetShaderType() const = 0;
		virtual const std::vector<std::vector<uint32_t>>& GetSPIRV() const = 0;
		virtual const ShaderReflection& GetReflection() const = 0;

		virtual const std::filesystem::path& GetSourcePath() const = 0;

#ifdef GRAVIX_EDITOR_BUILD
		// Create from source file (compiles using ShaderCompiler) - Editor only
		static Ref<Shader> Create(const std::filesystem::path& shaderPath, ShaderType type);
#endif

		// Create from pre-compiled SPIR-V (used by ShaderImporter and runtime)
		static Ref<Shader> Create(const std::filesystem::path& sourcePath, ShaderType type,
			const std::vector<std::vector<uint32_t>>& spirvData, const ShaderReflection& reflection);
	};

}
