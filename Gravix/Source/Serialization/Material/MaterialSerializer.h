#pragma once

#include "Reflections/ShaderReflection.h"

#include <filesystem>
#include <vector>

namespace Gravix 
{

	struct MaterialSerializedData
	{
		std::vector<std::vector<uint32_t>> SpirvCode;
		std::vector<uint8_t> PipelineCache;
		ShaderReflection Reflection;
	};

	class MaterialSerializer 
	{
	public:
		MaterialSerializer(MaterialSerializedData* serializedData)
			: m_SerializedData(serializedData) {}

		void Serialize(const std::filesystem::path& shaderFilePath, const std::filesystem::path& cacheFilePath);
		bool Deserialize(const std::filesystem::path& shaderFilePath, const std::filesystem::path& cacheFilePath);

		bool IsModified() { return m_IsModified; };
	private:
		MaterialSerializedData* m_SerializedData;

		bool m_IsModified;
	};

}