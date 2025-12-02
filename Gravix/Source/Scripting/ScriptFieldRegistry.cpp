#include "pch.h"
#include "ScriptFieldRegistry.h"

#include "Core/Log.h"

#ifdef GRAVIX_EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif
#include <fstream>
#include <glm/glm.hpp>

namespace Gravix
{

	// Helper functions to convert ScriptFieldType to/from string
	static const char* ScriptFieldTypeToString(ScriptFieldType type)
	{
		switch (type)
		{
			case ScriptFieldType::None:    return "None";
			case ScriptFieldType::Float:   return "Float";
			case ScriptFieldType::Double:  return "Double";
			case ScriptFieldType::Int:     return "Int";
			case ScriptFieldType::UInt:    return "UInt";
			case ScriptFieldType::Long:    return "Long";
			case ScriptFieldType::Short:   return "Short";
			case ScriptFieldType::Byte:    return "Byte";
			case ScriptFieldType::Char:    return "Char";
			case ScriptFieldType::Bool:    return "Bool";
			case ScriptFieldType::Vector2: return "Vector2";
			case ScriptFieldType::Vector3: return "Vector3";
			case ScriptFieldType::Vector4: return "Vector4";
			case ScriptFieldType::Entity:  return "Entity";
			default: return "None";
		}
	}

	static ScriptFieldType StringToScriptFieldType(const std::string& str)
	{
		if (str == "Float")   return ScriptFieldType::Float;
		if (str == "Double")  return ScriptFieldType::Double;
		if (str == "Int")     return ScriptFieldType::Int;
		if (str == "UInt")    return ScriptFieldType::UInt;
		if (str == "Long")    return ScriptFieldType::Long;
		if (str == "Short")   return ScriptFieldType::Short;
		if (str == "Byte")    return ScriptFieldType::Byte;
		if (str == "Char")    return ScriptFieldType::Char;
		if (str == "Bool")    return ScriptFieldType::Bool;
		if (str == "Vector2") return ScriptFieldType::Vector2;
		if (str == "Vector3") return ScriptFieldType::Vector3;
		if (str == "Vector4") return ScriptFieldType::Vector4;
		if (str == "Entity")  return ScriptFieldType::Entity;
		return ScriptFieldType::None;
	}

	EntityScriptData& ScriptFieldRegistry::GetEntityScriptData(UUID entityID)
	{
		return m_EntityScriptData[entityID];
	}

	bool ScriptFieldRegistry::HasEntityScriptData(UUID entityID) const
	{
		return m_EntityScriptData.find(entityID) != m_EntityScriptData.end();
	}

	ScriptInstanceData* ScriptFieldRegistry::GetScriptInstanceData(UUID entityID, const std::string& scriptName)
	{
		auto it = m_EntityScriptData.find(entityID);
		if (it == m_EntityScriptData.end())
			return nullptr;

		for (auto& scriptData : it->second.Scripts)
		{
			if (scriptData.ScriptName == scriptName)
				return &scriptData;
		}

		return nullptr;
	}

	void ScriptFieldRegistry::SetFieldValue(UUID entityID, const std::string& scriptName, const std::string& fieldName, const ScriptFieldValue& value)
	{
		auto& entityData = GetEntityScriptData(entityID);
		entityData.EntityID = entityID;

		// Find or create script instance data
		ScriptInstanceData* scriptData = nullptr;
		for (auto& script : entityData.Scripts)
		{
			if (script.ScriptName == scriptName)
			{
				scriptData = &script;
				break;
			}
		}

		if (!scriptData)
		{
			entityData.Scripts.push_back(ScriptInstanceData{ scriptName, {} });
			scriptData = &entityData.Scripts.back();
		}

		// Set field value
		scriptData->Fields[fieldName] = value;
	}

	ScriptFieldValue* ScriptFieldRegistry::GetFieldValue(UUID entityID, const std::string& scriptName, const std::string& fieldName)
	{
		auto scriptData = GetScriptInstanceData(entityID, scriptName);
		if (!scriptData)
			return nullptr;

		auto it = scriptData->Fields.find(fieldName);
		if (it == scriptData->Fields.end())
			return nullptr;

		return &it->second;
	}

	void ScriptFieldRegistry::RemoveEntity(UUID entityID)
	{
		m_EntityScriptData.erase(entityID);
	}

	void ScriptFieldRegistry::Clear()
	{
		m_EntityScriptData.clear();
	}

#ifdef GRAVIX_EDITOR_BUILD
	void ScriptFieldRegistry::Serialize(const std::filesystem::path& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "ScriptRegistry" << YAML::Value;

		out << YAML::BeginSeq;
		for (const auto& [entityID, entityData] : m_EntityScriptData)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Entity" << YAML::Value << (uint64_t)entityID;
			out << YAML::Key << "Scripts" << YAML::Value << YAML::BeginSeq;

			for (const auto& scriptData : entityData.Scripts)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "ScriptName" << YAML::Value << scriptData.ScriptName;
				out << YAML::Key << "Fields" << YAML::Value << YAML::BeginSeq;

				for (const auto& [fieldName, fieldValue] : scriptData.Fields)
				{
					out << YAML::BeginMap;
					out << YAML::Key << "Name" << YAML::Value << fieldName;
					out << YAML::Key << "Type" << YAML::Value << ScriptFieldTypeToString(fieldValue.Type);

					// Serialize value based on type
					out << YAML::Key << "Value" << YAML::Value;
					switch (fieldValue.Type)
					{
					case ScriptFieldType::Float:
						out << fieldValue.GetValue<float>();
						break;
					case ScriptFieldType::Double:
						out << fieldValue.GetValue<double>();
						break;
					case ScriptFieldType::Int:
						out << fieldValue.GetValue<int32_t>();
						break;
					case ScriptFieldType::UInt:
						out << fieldValue.GetValue<uint32_t>();
						break;
					case ScriptFieldType::Long:
						out << fieldValue.GetValue<int64_t>();
						break;
					case ScriptFieldType::Short:
						out << fieldValue.GetValue<int16_t>();
						break;
					case ScriptFieldType::Byte:
						out << fieldValue.GetValue<uint8_t>();
						break;
					case ScriptFieldType::Char:
						out << fieldValue.GetValue<char>();
						break;
					case ScriptFieldType::Bool:
						out << fieldValue.GetValue<bool>();
						break;
					case ScriptFieldType::Vector2:
					{
						auto vec = fieldValue.GetValue<glm::vec2>();
						out << YAML::Flow << YAML::BeginSeq << vec.x << vec.y << YAML::EndSeq;
						break;
					}
					case ScriptFieldType::Vector3:
					{
						auto vec = fieldValue.GetValue<glm::vec3>();
						out << YAML::Flow << YAML::BeginSeq << vec.x << vec.y << vec.z << YAML::EndSeq;
						break;
					}
					case ScriptFieldType::Vector4:
					{
						auto vec = fieldValue.GetValue<glm::vec4>();
						out << YAML::Flow << YAML::BeginSeq << vec.x << vec.y << vec.z << vec.w << YAML::EndSeq;
						break;
					}
					case ScriptFieldType::Entity:
						out << (uint64_t)fieldValue.GetValue<UUID>();
						break;
					}

					out << YAML::EndMap;
				}

				out << YAML::EndSeq; // Fields
				out << YAML::EndMap; // Script
			}

			out << YAML::EndSeq; // Scripts
			out << YAML::EndMap; // Entity
		}
		out << YAML::EndSeq; // ScriptRegistry
		out << YAML::EndMap;

		// Write to file
		std::ofstream fout(filepath);
		fout << out.c_str();
		fout.close();

		GX_CORE_INFO("Script registry saved to: {0}", filepath.string());
	}

	void ScriptFieldRegistry::Deserialize(const std::filesystem::path& filepath)
	{
		if (!std::filesystem::exists(filepath))
		{
			GX_CORE_WARN("Script registry file not found: {0}", filepath.string());
			return;
		}

		std::ifstream stream(filepath);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["ScriptRegistry"])
			return;

		auto scriptRegistry = data["ScriptRegistry"];
		for (auto entityNode : scriptRegistry)
		{
			UUID entityID = entityNode["Entity"].as<uint64_t>();
			auto& entityData = GetEntityScriptData(entityID);
			entityData.EntityID = entityID;

			auto scriptsNode = entityNode["Scripts"];
			for (auto scriptNode : scriptsNode)
			{
				ScriptInstanceData scriptData;
				scriptData.ScriptName = scriptNode["ScriptName"].as<std::string>();

				auto fieldsNode = scriptNode["Fields"];
				for (auto fieldNode : fieldsNode)
				{
					std::string fieldName = fieldNode["Name"].as<std::string>();
					ScriptFieldValue fieldValue;
					fieldValue.Type = StringToScriptFieldType(fieldNode["Type"].as<std::string>());

					// Deserialize value based on type
					auto valueNode = fieldNode["Value"];
					switch (fieldValue.Type)
					{
					case ScriptFieldType::Float:
						fieldValue.SetValue(valueNode.as<float>());
						break;
					case ScriptFieldType::Double:
						fieldValue.SetValue(valueNode.as<double>());
						break;
					case ScriptFieldType::Int:
						fieldValue.SetValue(valueNode.as<int32_t>());
						break;
					case ScriptFieldType::UInt:
						fieldValue.SetValue(valueNode.as<uint32_t>());
						break;
					case ScriptFieldType::Long:
						fieldValue.SetValue(valueNode.as<int64_t>());
						break;
					case ScriptFieldType::Short:
						fieldValue.SetValue(valueNode.as<int16_t>());
						break;
					case ScriptFieldType::Byte:
						fieldValue.SetValue(valueNode.as<uint8_t>());
						break;
					case ScriptFieldType::Char:
						fieldValue.SetValue(valueNode.as<char>());
						break;
					case ScriptFieldType::Bool:
						fieldValue.SetValue(valueNode.as<bool>());
						break;
					case ScriptFieldType::Vector2:
					{
						glm::vec2 vec;
						vec.x = valueNode[0].as<float>();
						vec.y = valueNode[1].as<float>();
						fieldValue.SetValue(vec);
						break;
					}
					case ScriptFieldType::Vector3:
					{
						glm::vec3 vec;
						vec.x = valueNode[0].as<float>();
						vec.y = valueNode[1].as<float>();
						vec.z = valueNode[2].as<float>();
						fieldValue.SetValue(vec);
						break;
					}
					case ScriptFieldType::Vector4:
					{
						glm::vec4 vec;
						vec.x = valueNode[0].as<float>();
						vec.y = valueNode[1].as<float>();
						vec.z = valueNode[2].as<float>();
						vec.w = valueNode[3].as<float>();
						fieldValue.SetValue(vec);
						break;
					}
					case ScriptFieldType::Entity:
						fieldValue.SetValue(UUID(valueNode.as<uint64_t>()));
						break;
					}

					scriptData.Fields[fieldName] = fieldValue;
				}

				entityData.Scripts.push_back(scriptData);
			}
		}

		GX_CORE_INFO("Script registry loaded from: {0}", filepath.string());
	}
#endif // GRAVIX_EDITOR_BUILD

}
