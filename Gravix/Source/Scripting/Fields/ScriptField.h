#pragma once

#include "Scripting/Core/ScriptTypes.h"
#include "Core/UUID.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace Gravix
{
	// Stores the value of a script field
	struct ScriptFieldValue
	{
		ScriptFieldType Type = ScriptFieldType::None;
		uint8_t Data[16] = { 0 }; // Max size for Vector4/Entity

		ScriptFieldValue() = default;

		template<typename T>
		T GetValue() const
		{
			return *(T*)Data;
		}

		template<typename T>
		void SetValue(const T& value)
		{
			memcpy(Data, &value, sizeof(T));
		}
	};

	struct ScriptField
	{
		std::string Name;
		ScriptFieldType Type = ScriptFieldType::None;
		uint32_t Size = 0;
		uint32_t Offset = 0;

		// Attributes
		bool HasSerializeField = false; // [SerializeField] attribute
		bool HasRange = false;          // [Range] attribute
		float RangeMin = 0.0f;
		float RangeMax = 0.0f;

		// Default value
		ScriptFieldValue DefaultValue;

		ScriptField() = default;
		ScriptField(const std::string& name, ScriptFieldType type, uint32_t size, uint32_t offset)
			: Name(name), Type(type), Size(size), Offset(offset) {}
	};

	// Stores all field values for a single script instance
	struct ScriptInstanceData
	{
		std::string ScriptName;
		std::unordered_map<std::string, ScriptFieldValue> Fields;
	};

	// Stores all script instances for a single entity
	struct EntityScriptData
	{
		UUID EntityID;
		std::vector<ScriptInstanceData> Scripts;
	};

}
