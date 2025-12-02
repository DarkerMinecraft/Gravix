#include "pch.h"
#include "ScriptTypes.h"

#include <mono/metadata/class.h>

namespace Gravix
{

	const std::unordered_map<std::string, ScriptFieldType>& ScriptTypeUtils::GetTypeMap()
	{
		static std::unordered_map<std::string, ScriptFieldType> s_TypeMap =
		{
			{ "System.Single", ScriptFieldType::Float },
			{ "System.Double", ScriptFieldType::Double },
			{ "System.Int32", ScriptFieldType::Int },
			{ "System.UInt32", ScriptFieldType::UInt },
			{ "System.Int64", ScriptFieldType::Long },
			{ "System.Int16", ScriptFieldType::Short },
			{ "System.Byte", ScriptFieldType::Byte },
			{ "System.Char", ScriptFieldType::Char },
			{ "System.Boolean", ScriptFieldType::Bool },

			{ "GravixEngine.Vector2", ScriptFieldType::Vector2 },
			{ "GravixEngine.Vector3", ScriptFieldType::Vector3 },
			{ "GravixEngine.Vector4", ScriptFieldType::Vector4 },

			{ "GravixEngine.Entity", ScriptFieldType::Entity },
		};

		return s_TypeMap;
	}

	ScriptFieldType ScriptTypeUtils::MonoTypeToScriptType(MonoType* monoType)
	{
		std::string typeName = mono_type_get_name(monoType);

		const auto& typeMap = GetTypeMap();
		auto it = typeMap.find(typeName);
		if (it != typeMap.end())
			return it->second;

		return ScriptFieldType::None;
	}

	const char* ScriptTypeUtils::ScriptFieldTypeToString(ScriptFieldType type)
	{
		switch (type)
		{
		case ScriptFieldType::Float:   return "Float";
		case ScriptFieldType::Double:  return "Double";
		case ScriptFieldType::Vector2: return "Vector2";
		case ScriptFieldType::Vector3: return "Vector3";
		case ScriptFieldType::Vector4: return "Vector4";
		case ScriptFieldType::Int:     return "Int";
		case ScriptFieldType::UInt:    return "UInt";
		case ScriptFieldType::Long:    return "Long";
		case ScriptFieldType::Bool:    return "Boolean";
		case ScriptFieldType::Short:   return "Short";
		case ScriptFieldType::Byte:    return "Byte";
		case ScriptFieldType::Char:    return "Char";
		case ScriptFieldType::Entity:  return "Entity";
		default:                       return "Unknown";
		}
	}

}
