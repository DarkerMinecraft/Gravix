#include "pch.h"
#include "MaterialSerializer.h"

#include "Serialization/BinarySerializer.h"
#include "Serialization/BinaryDeserializer.h"

namespace Gravix 
{

	void MaterialSerializer::Serialize(const std::filesystem::path& shaderFilePath, const std::filesystem::path& cacheFilePath)
	{
		BinarySerializer serializer(0);
		serializer.Write((int64_t)std::filesystem::last_write_time(shaderFilePath).time_since_epoch().count());
		serializer.Write(m_SerializedData->SpirvCode);
		serializer.Write(m_SerializedData->PipelineCache);
		serializer.Write(m_SerializedData->Reflection);

		serializer.WriteToFile(cacheFilePath);
	}

	bool MaterialSerializer::Deserialize(const std::filesystem::path& shaderFilePath, const std::filesystem::path& cacheFilePath)
	{
		BinaryDeserializer deserializer(cacheFilePath, 0);

		int64_t cachedTimestamp = deserializer.Read<int64_t>();
		int64_t currentTimestamp = std::filesystem::last_write_time(shaderFilePath).time_since_epoch().count();

		m_IsModified = (cachedTimestamp != currentTimestamp);
		m_SerializedData->SpirvCode = deserializer.ReadVector<std::vector<uint32_t>>();
		m_SerializedData->PipelineCache = deserializer.ReadVector<uint8_t>();
		m_SerializedData->Reflection = deserializer.Read<ShaderReflection>();

		return true;
	}

}