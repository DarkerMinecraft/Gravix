#include "pch.h"
#include "VulkanShader.h"

#ifdef GRAVIX_EDITOR_BUILD
#include "Utils/ShaderCompilerSystem.h"
#endif
#include "Serialization/BinarySerializer.h"
#include "Serialization/BinaryDeserializer.h"
#include "Project/Project.h"

namespace Gravix
{

#ifdef GRAVIX_EDITOR_BUILD
	VulkanShader::VulkanShader(const std::filesystem::path& shaderPath, ShaderType type)
		: m_SourcePath(shaderPath), m_Type(type)
	{
		std::filesystem::path cachePath = GetCachePath(shaderPath);

		// Check if cached version exists and is up-to-date
		bool needsRecompile = true;
		if (std::filesystem::exists(cachePath))
		{
			auto cacheTime = std::filesystem::last_write_time(cachePath);
			auto sourceTime = std::filesystem::last_write_time(shaderPath);

			if (cacheTime >= sourceTime)
			{
				LoadCachedShader(cachePath);
				needsRecompile = false;
			}
		}

		if (needsRecompile)
		{
			GX_CORE_INFO("Compiling shader: {0}", shaderPath.string());
			CompileShader(shaderPath);
			SaveCachedShader(cachePath);
		}
		else
		{
			GX_CORE_INFO("Loaded cached shader: {0}", shaderPath.string());
		}
	}

	void VulkanShader::CompileShader(const std::filesystem::path& shaderPath)
	{
		bool success = ShaderCompilerSystem::Get().CompileShader(shaderPath, &m_SPIRVCode, &m_Reflection);

		if (!success)
		{
			GX_CORE_ERROR("Failed to compile shader: {0}", shaderPath.string());
			m_SPIRVCode.clear();
		}
	}

	void VulkanShader::LoadCachedShader(const std::filesystem::path& cachePath)
	{
		BinaryDeserializer deserializer(cachePath, 1);

		// Read SPIR-V code
		uint32_t spirvCount = deserializer.Read<uint32_t>();
		m_SPIRVCode.resize(spirvCount);
		for (auto& spirv : m_SPIRVCode)
		{
			uint32_t size = deserializer.Read<uint32_t>();
			spirv.resize(size);
			deserializer.ReadBytes(spirv.data(), size * sizeof(uint32_t));
		}

		// Read reflection data
		m_Reflection.Deserialize(deserializer);
	}

	void VulkanShader::SaveCachedShader(const std::filesystem::path& cachePath)
	{
		// Ensure cache directory exists
		std::filesystem::create_directories(cachePath.parent_path());

		BinarySerializer serializer(1);

		// Write SPIR-V code
		serializer.Write(static_cast<uint32_t>(m_SPIRVCode.size()));
		for (const auto& spirv : m_SPIRVCode)
		{
			serializer.Write(static_cast<uint32_t>(spirv.size()));
			serializer.WriteBytes(spirv.data(), spirv.size() * sizeof(uint32_t));
		}

		// Write reflection data
		m_Reflection.Serialize(serializer);

		serializer.WriteToFile(cachePath);
		GX_CORE_INFO("Saved shader cache: {0}", cachePath.string());
	}

	std::filesystem::path VulkanShader::GetCachePath(const std::filesystem::path& shaderPath) const
	{
		// Get relative path from Assets directory
		std::filesystem::path relativePath = std::filesystem::relative(shaderPath, Project::GetAssetDirectory());

		// Create cache path in Library/ShaderCache/
		std::filesystem::path cachePath = Project::GetLibraryDirectory() / "ShaderCache" / relativePath;
		cachePath.replace_extension(".shadercache");

		return cachePath;
	}
#endif // GRAVIX_EDITOR_BUILD

	VulkanShader::VulkanShader(const std::filesystem::path& sourcePath, ShaderType type,
		const std::vector<std::vector<uint32_t>>& spirvData, const ShaderReflection& reflection)
		: m_SourcePath(sourcePath), m_Type(type), m_SPIRVCode(spirvData), m_Reflection(reflection)
	{
		GX_CORE_INFO("Created shader from pre-compiled SPIR-V: {0}", sourcePath.string());
	}

}
