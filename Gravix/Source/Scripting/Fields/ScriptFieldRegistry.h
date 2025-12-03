#pragma once

#include "Scripting/Fields/ScriptField.h"
#include "Core/UUID.h"

#include <filesystem>
#include <unordered_map>

namespace Gravix
{

	class ScriptFieldRegistry
	{
	public:
		ScriptFieldRegistry() = default;
		~ScriptFieldRegistry() = default;

		// Get or create entity script data
		EntityScriptData& GetEntityScriptData(UUID entityID);

		// Check if entity has script data
		bool HasEntityScriptData(UUID entityID) const;

		// Get script instance data for a specific script on an entity
		ScriptInstanceData* GetScriptInstanceData(UUID entityID, const std::string& scriptName);

		// Set field value for a script instance
		void SetFieldValue(UUID entityID, const std::string& scriptName, const std::string& fieldName, const ScriptFieldValue& value);

		// Get field value for a script instance
		ScriptFieldValue* GetFieldValue(UUID entityID, const std::string& scriptName, const std::string& fieldName);

		// Remove entity from registry
		void RemoveEntity(UUID entityID);

		// Clear all data
		void Clear();

#ifdef GRAVIX_EDITOR_BUILD
		// Serialization (YAML - Editor only)
		void Serialize(const std::filesystem::path& filepath);
		void Deserialize(const std::filesystem::path& filepath);
#endif

		// Get all entity data (for iteration)
		const std::unordered_map<UUID, EntityScriptData>& GetAllEntityData() const { return m_EntityScriptData; }

	private:
		std::unordered_map<UUID, EntityScriptData> m_EntityScriptData;
	};

}
