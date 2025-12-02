#pragma once

#include "ScriptEngine.h"
#include "ScriptFieldRegistry.h"

#include <filesystem>

namespace Gravix
{

	// Runtime Script Engine - Loads combined assemblies and field registry
	// Used in packaged builds with PaK files
	// NOTE: Currently commented out - requires PaK builder implementation
	class RuntimeScriptEngine : public ScriptEngine
	{
	public:
		// TODO: Implement when PaK builder is ready
		// static void Initialize(const std::filesystem::path& combinedAssemblyPath, const ScriptFieldRegistry& fieldRegistry);
		// static void Shutdown();

		// static void OnRuntimeStart(Scene* scene);
		// static void OnRuntimeStop();

		// static void OnCreateEntity(Entity entity);
		// static void OnUpdateEntity(Entity entity, float deltaTime);

		// static Scene* GetSceneContext();

		// static std::unordered_map<std::string, Ref<ScriptClass>>& GetEntityClasses();
		// static bool IsEntityClassExists(const std::string& fullClassName);

		// // Field registry access
		// static ScriptFieldRegistry& GetFieldRegistry();

		// // Get script instances for an entity
		// static std::vector<Ref<ScriptInstance>>* GetEntityScriptInstances(UUID entityID);

		// // Field value access
		// static bool GetFieldValue(MonoObject* instance, const ScriptField& field, ScriptFieldValue& outValue);
		// static bool SetFieldValue(MonoObject* instance, const ScriptField& field, const ScriptFieldValue& value);

	private:
		// static void InitMono();
		// static void ShutdownMono();

		// static void LoadCombinedAssembly(const std::filesystem::path& assemblyPath);
		// static void LoadFieldRegistryFromPak(const ScriptFieldRegistry& fieldRegistry);

		// static MonoObject* InstantiateClass(MonoClass* monoClass);
		// static MonoImage* GetAssemblyImage();

		// friend class ScriptClass;
		// friend class ScriptGlue;
	};

}
