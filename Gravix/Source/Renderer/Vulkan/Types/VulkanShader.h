#pragma once

#include "Renderer/Generic/Types/Shader.h"
#include "Reflections/ShaderReflection.h"

#include <vector>
#include <filesystem>

namespace Gravix
{

	class VulkanShader : public Shader
	{
	public:
#ifdef GRAVIX_EDITOR_BUILD
		// Create from source (compiles with ShaderCompiler) - Editor only
		VulkanShader(const std::filesystem::path& shaderPath, ShaderType type);
#endif

		// Create from pre-compiled SPIR-V (used in both editor via ShaderImporter and runtime)
		VulkanShader(const std::filesystem::path& sourcePath, ShaderType type,
			const std::vector<std::vector<uint32_t>>& spirvData, const ShaderReflection& reflection);

		virtual ~VulkanShader() = default;

		virtual ShaderType GetShaderType() const override { return m_Type; }
		virtual const std::vector<std::vector<uint32_t>>& GetSPIRV() const override { return m_SPIRVCode; }
		virtual const ShaderReflection& GetReflection() const override { return m_Reflection; }

		virtual const std::filesystem::path& GetSourcePath() const override { return m_SourcePath; }

	private:
#ifdef GRAVIX_EDITOR_BUILD
		void CompileShader(const std::filesystem::path& shaderPath);
		void LoadCachedShader(const std::filesystem::path& cachePath);
		void SaveCachedShader(const std::filesystem::path& cachePath);

		std::filesystem::path GetCachePath(const std::filesystem::path& shaderPath) const;
#endif

	private:
		std::filesystem::path m_SourcePath;
		ShaderType m_Type;

		std::vector<std::vector<uint32_t>> m_SPIRVCode;
		ShaderReflection m_Reflection;
	};

}
