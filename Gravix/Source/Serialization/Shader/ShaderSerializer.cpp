#include "pch.h"
#include "ShaderSerializer.h"

#include "Serialization/BinarySerializer.h"
#include "Serialization/BinaryDeserializer.h"

namespace Gravix 
{

	void ShaderSerializer::Serialize(const std::filesystem::path& shaderFilePath, const std::filesystem::path& cacheFilePath)
	{
		BinarySerializer serializer(0);
		serializer.Write((int64_t)std::filesystem::last_write_time(shaderFilePath).time_since_epoch().count());
		serializer.Write(m_SerializedData->SpirvCode);
		serializer.Write(m_SerializedData->PipelineCache);
		serializer.Write(m_SerializedData->Reflection);

		serializer.WriteToFile(cacheFilePath);
	}

	bool ShaderSerializer::Deserialize(const std::filesystem::path& shaderFilePath, const std::filesystem::path& cacheFilePath)
	{
		m_ShaderFilePath = shaderFilePath;

		BinaryDeserializer deserializer(cacheFilePath, 0);
		m_LastModified = deserializer.Read<int64_t>();
		m_SerializedData->SpirvCode = deserializer.ReadVector<std::vector<uint32_t>>();
		m_SerializedData->PipelineCache = deserializer.ReadVector<uint8_t>();
		m_SerializedData->Reflection = deserializer.Read<ShaderReflection>();

		return true;
	}

}