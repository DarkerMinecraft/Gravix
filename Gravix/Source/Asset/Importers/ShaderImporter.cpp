#include "pch.h"
#include "ShaderImporter.h"

#include "Utils/ShaderCompilerSystem.h"
#include "Project/Project.h"
#include "Reflections/ShaderReflection.h"

#include <fstream>
#include <sstream>

namespace Gravix
{

	Ref<Shader> ShaderImporter::ImportShader(AssetHandle handle, const AssetMetadata& metadata)
	{
		std::filesystem::path fullPath = Project::GetAssetDirectory() / metadata.FilePath;

		if (!std::filesystem::exists(fullPath))
		{
			GX_CORE_ERROR("Shader file not found: {0}", fullPath.string());
			return nullptr;
		}

		// Detect shader type from file
		ShaderType type = DetectShaderType(fullPath);

		// Use global ShaderCompiler system (editor only)
		std::vector<std::vector<uint32_t>> spirvCode;
		ShaderReflection reflection;

		bool success = ShaderCompilerSystem::Get().CompileShader(fullPath, &spirvCode, &reflection);

		if (!success || spirvCode.empty())
		{
			GX_CORE_ERROR("Failed to compile shader: {0}", fullPath.string());
			return nullptr;
		}

		// Create shader from compiled SPIR-V
		Ref<Shader> shader = Shader::Create(fullPath, type, spirvCode, reflection);

		GX_CORE_INFO("Imported shader: {0} (Type: {1})", fullPath.string(),
			type == ShaderType::Graphics ? "Graphics" : "Compute");

		return shader;
	}

	Ref<Shader> ShaderImporter::LoadFromFile(const std::filesystem::path& shaderPath, ShaderType type)
	{
		if (!std::filesystem::exists(shaderPath))
		{
			GX_CORE_ERROR("Shader file not found: {0}", shaderPath.string());
			return nullptr;
		}

		// Auto-detect shader type if not explicitly provided
		if (type == ShaderType::Graphics)
		{
			ShaderType detectedType = DetectShaderType(shaderPath);
			if (detectedType != ShaderType::Graphics)
				type = detectedType;
		}

		// Use global ShaderCompiler system (editor only)
		std::vector<std::vector<uint32_t>> spirvCode;
		ShaderReflection reflection;

		bool success = ShaderCompilerSystem::Get().CompileShader(shaderPath, &spirvCode, &reflection);

		if (!success || spirvCode.empty())
		{
			GX_CORE_ERROR("Failed to compile shader: {0}", shaderPath.string());
			return nullptr;
		}

		// Create shader from compiled SPIR-V
		Ref<Shader> shader = Shader::Create(shaderPath, type, spirvCode, reflection);

		GX_CORE_INFO("Loaded shader from file: {0} (Type: {1})", shaderPath.string(),
			type == ShaderType::Graphics ? "Graphics" : "Compute");

		return shader;
	}

	ShaderType ShaderImporter::DetectShaderType(const std::filesystem::path& shaderPath)
	{
		// Open file and check for compute shader entry point or keywords
		std::ifstream file(shaderPath);
		if (!file.is_open())
		{
			GX_CORE_WARN("Could not open shader file for type detection: {0}", shaderPath.string());
			return ShaderType::Graphics; // Default to graphics
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string content = buffer.str();

		// Check for compute shader indicators
		if (content.find("[shader(\"compute\")]") != std::string::npos ||
			content.find("DispatchThreadID") != std::string::npos ||
			content.find("GroupThreadID") != std::string::npos ||
			content.find("computeMain") != std::string::npos)
		{
			return ShaderType::Compute;
		}

		// Check for graphics shader indicators
		if (content.find("[shader(\"vertex\")]") != std::string::npos ||
			content.find("[shader(\"fragment\")]") != std::string::npos ||
			content.find("vertexMain") != std::string::npos ||
			content.find("fragmentMain") != std::string::npos)
		{
			return ShaderType::Graphics;
		}

		// Default to graphics if unclear
		return ShaderType::Graphics;
	}

}
