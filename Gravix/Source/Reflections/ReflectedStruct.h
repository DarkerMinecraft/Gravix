#pragma once

#include "Core/Log.h"

#include "Serialization/BinarySerializer.h"
#include "Serialization/BinaryDeserializer.h"

#include <string>
#include <vector>

namespace Gravix
{
	struct ReflectedStructMember
	{
		std::string Name;
		size_t Offset;
		size_t Size;

		void Serialize(BinarySerializer& serializer) 
		{
			serializer.Write(Name);
			serializer.Write(Offset);
			serializer.Write(Size);
		}

		void Deserialize(BinaryDeserializer& deserializer) 
		{
			Name = deserializer.ReadString();
			Offset = deserializer.Read<size_t>();
			Size = deserializer.Read<size_t>();
		}
	};

	struct ReflectedStruct
	{
		std::string Name;
		size_t Size;
		std::vector<ReflectedStructMember> Members;

		std::vector<uint8_t> CreateInstance() 
		{
			return std::vector<uint8_t>(Size);
		}

		std::vector<ReflectedStructMember>& GetFields() { return Members; }
		size_t GetSize() const { return Size; }

		template<typename T>
		void SetField(std::vector<uint8_t>& instance, const std::string& fieldName, const T& value)
		{
			auto it = std::find_if(Members.begin(), Members.end(), [&](const ReflectedStructMember& member) {
				return member.Name == fieldName;
				});
			if (it != Members.end())
			{
				if (it->Size != sizeof(T))
				{
					GX_CORE_ERROR("Size mismatch when setting field '{}' in struct '{}'. Expected size: {}, provided size: {}",
						fieldName, Name, it->Size, sizeof(T));
					return;
				}
				std::memcpy(instance.data() + it->Offset, &value, sizeof(T));
			}
			else
			{
				GX_CORE_ERROR("Field '{}' not found in struct '{}'", fieldName, Name);
			}
		}

		void Serialize(BinarySerializer& serializer) 
		{
			serializer.Write(Name);
			serializer.Write(Size);
			serializer.Write(Members);
		}

		void Deserialize(BinaryDeserializer& deserializer) 
		{
			Name = deserializer.ReadString();
			Size = deserializer.Read<size_t>();
			Members = deserializer.ReadVector<ReflectedStructMember>();
		}
	};
	
}