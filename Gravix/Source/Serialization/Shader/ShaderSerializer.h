#pragma once

#include "Reflections/ShaderReflection.h"

#include <filesystem>
#include <vector>

namespace Gravix 
{

	struct ShaderSerializedData
	{
		std::vector<std::vector<uint32_t>> SpirvCode;
		std::vector<uint8_t> PipelineCache;
		ShaderReflection Reflection;
	};

	class ShaderSerializer 
	{
	public:
		ShaderSerializer(ShaderSerializedData* serializedData)
			: m_SerializedData(serializedData) {}

		void Serialize(const std::filesystem::path& shaderFilePath, const std::filesystem::path& cacheFilePath);
		bool Deserialize(const std::filesystem::path& shaderFilePath, const std::filesystem::path& cacheFilePath);

		bool IsModified() { return std::filesystem::last_write_time(m_ShaderFilePath).time_since_epoch().count() != m_LastModified;  }
	private:
		ShaderSerializedData* m_SerializedData;

		int64_t m_LastModified;
		std::filesystem::path m_ShaderFilePath;
	};

}