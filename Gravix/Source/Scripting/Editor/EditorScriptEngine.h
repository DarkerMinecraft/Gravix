#pragma once

#include "Scripting/Core/ScriptEngine.h"

#include <filesystem>

namespace Gravix
{

	// Editor Script Engine - Loads Core and App assemblies separately
	// Used during development in the editor
	class EditorScriptEngine : public ScriptEngine
	{
	public:
		static void Initialize();
		static void Shutdown();

		static void LoadCoreAssembly(const std::filesystem::path& coreAssemblyPath);
		static void LoadAppAssembly(const std::filesystem::path& appAssemblyPath);

		static void OnRuntimeStart(Scene* scene);
		static void OnRuntimeStop();

		static void OnCreateEntity(Entity entity);
		static void OnUpdateEntity(Entity entity, float deltaTime);
		static void OnDestroyEntity(Entity entity);

		static Scene* GetSceneContext();

		static std::unordered_map<std::string, Ref<ScriptClass>>& GetEntityClasses();
		static bool IsEntityClassExists(const std::string& fullClassName);

		// Field registry access
		static ScriptFieldRegistry& GetFieldRegistry();

		// Get script instances for an entity
		static std::vector<Ref<ScriptInstance>>* GetEntityScriptInstances(UUID entityID);

		// Field value access
		static bool GetFieldValue(MonoObject* instance, const ScriptField& field, ScriptFieldValue& outValue);
		static bool SetFieldValue(MonoObject* instance, const ScriptField& field, const ScriptFieldValue& value);

	public:
		// Internal access for ScriptClass/ScriptInstance
		static MonoImage* GetCoreAssemblyImage();
		static MonoImage* GetAppAssemblyImage();
		static MonoObject* InstantiateClass(MonoClass* monoClass);

	private:
		static void InitMono();
		static void ShutdownMono();

		static void LoadAssemblyClasses(MonoImage* image);

		friend class ScriptClass;
		friend class ScriptGlue;
	};

}
