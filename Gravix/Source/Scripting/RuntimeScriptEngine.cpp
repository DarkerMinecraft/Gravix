#include "pch.h"
#include "RuntimeScriptEngine.h"

// TODO: Uncomment when PaK builder is ready

/*
#include "ScriptGlue.h"
#include "ScriptUtils.h"
#include "ScriptTypes.h"
#include "ScriptFieldHandler.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/class.h>

#include <glm/glm.hpp>

namespace Gravix
{

	struct RuntimeScriptEngineData
	{
		MonoDomain* RootDomain = nullptr;
		MonoDomain* AppDomain = nullptr;

		MonoAssembly* CombinedAssembly = nullptr;
		MonoImage* CombinedAssemblyImage = nullptr;

		Scene* SceneContext = nullptr;

		Ref<ScriptClass> EntityClass;

		std::unordered_map<std::string, Ref<ScriptClass>> EntityClasses;
		std::unordered_map<UUID, std::vector<Ref<ScriptInstance>>> EntityInstances;

		ScriptFieldRegistry FieldRegistry;
	};

	static Ref<RuntimeScriptEngineData> s_RuntimeData = nullptr;

	void RuntimeScriptEngine::Initialize(const std::filesystem::path& combinedAssemblyPath, const ScriptFieldRegistry& fieldRegistry)
	{
		s_RuntimeData = CreateRef<RuntimeScriptEngineData>();

		InitMono();
		LoadCombinedAssembly(combinedAssemblyPath);
		LoadFieldRegistryFromPak(fieldRegistry);
		ScriptGlue::RegisterFunctions();
	}

	void RuntimeScriptEngine::Shutdown()
	{
		ShutdownMono();
		s_RuntimeData = nullptr;
	}

	void RuntimeScriptEngine::LoadCombinedAssembly(const std::filesystem::path& assemblyPath)
	{
		s_RuntimeData->CombinedAssembly = ScriptUtils::LoadMonoAssembly(assemblyPath);
		s_RuntimeData->CombinedAssemblyImage = mono_assembly_get_image(s_RuntimeData->CombinedAssembly);

		s_RuntimeData->EntityClass = CreateRef<ScriptClass>("GravixEngine", "Entity");

		// Load all entity classes from combined assembly
		const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(s_RuntimeData->CombinedAssemblyImage, MONO_TABLE_TYPEDEF);
		int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

		for (int32_t i = 0; i < numTypes; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

			const char* nameSpace = mono_metadata_string_heap(s_RuntimeData->CombinedAssemblyImage, cols[MONO_TYPEDEF_NAMESPACE]);
			const char* name = mono_metadata_string_heap(s_RuntimeData->CombinedAssemblyImage, cols[MONO_TYPEDEF_NAME]);

			MonoClass* monoClass = mono_class_from_name(s_RuntimeData->CombinedAssemblyImage, nameSpace, name);
			MonoClass* entityClass = mono_class_from_name(s_RuntimeData->CombinedAssemblyImage, "GravixEngine", "Entity");

			if (monoClass == entityClass)
				continue;

			bool isEntity = mono_class_is_subclass_of(monoClass, entityClass, false);
			if (!isEntity)
				continue;

			std::string fullName;
			if(strlen(nameSpace) != 0)
				fullName = fmt::format("{}.{}", nameSpace, name);
			else
				fullName = name;

			Ref<ScriptClass> scriptClass = CreateRef<ScriptClass>(nameSpace, name);
			s_RuntimeData->EntityClasses[fullName] = scriptClass;

			GX_CORE_INFO("Loaded Runtime Script Class: {0}", fullName);
		}
	}

	void RuntimeScriptEngine::LoadFieldRegistryFromPak(const ScriptFieldRegistry& fieldRegistry)
	{
		// Copy the field registry from the PaK file
		s_RuntimeData->FieldRegistry = fieldRegistry;
	}

	void RuntimeScriptEngine::OnRuntimeStart(Scene* scene)
	{
		s_RuntimeData->SceneContext = scene;
	}

	void RuntimeScriptEngine::OnRuntimeStop()
	{
		s_RuntimeData->EntityInstances.clear();
		s_RuntimeData->SceneContext = nullptr;
	}

	void RuntimeScriptEngine::OnCreateEntity(Entity entity)
	{
		UUID entityID = entity.GetID();
		auto& instances = s_RuntimeData->EntityInstances[entityID];
		instances.clear();

		auto scriptComponents = entity.GetComponents<ScriptComponent>();

		for (auto* scriptComponent : scriptComponents)
		{
			if (!IsEntityClassExists(scriptComponent->Name))
			{
				GX_CORE_WARN("Script class not found: {0}", scriptComponent->Name);
				continue;
			}

			auto scriptClass = s_RuntimeData->EntityClasses[scriptComponent->Name];
			auto instance = CreateRef<ScriptInstance>(scriptClass, entity);
			instances.push_back(instance);
			instance->InvokeOnCreate();
		}
	}

	void RuntimeScriptEngine::OnUpdateEntity(Entity entity, float deltaTime)
	{
		UUID entityID = entity.GetID();

		auto it = s_RuntimeData->EntityInstances.find(entityID);
		if (it == s_RuntimeData->EntityInstances.end())
			return;

		for (auto& instance : it->second)
			instance->InvokeOnUpdate(deltaTime);
	}

	Scene* RuntimeScriptEngine::GetSceneContext()
	{
		return s_RuntimeData->SceneContext;
	}

	std::unordered_map<std::string, Ref<ScriptClass>>& RuntimeScriptEngine::GetEntityClasses()
	{
		return s_RuntimeData->EntityClasses;
	}

	bool RuntimeScriptEngine::IsEntityClassExists(const std::string& fullClassName)
	{
		return s_RuntimeData->EntityClasses.contains(fullClassName);
	}

	void RuntimeScriptEngine::InitMono()
	{
		mono_set_assemblies_path("lib/mono/4.5");

		MonoDomain* domain = mono_jit_init("GravixJITRuntime");
		if (domain == nullptr)
		{
			GX_VERIFY("Failed to initialize Mono JIT runtime!");
		}

		s_RuntimeData->RootDomain = domain;

		s_RuntimeData->AppDomain = mono_domain_create_appdomain((char*)"GravixScriptRuntime", nullptr);
		mono_domain_set(s_RuntimeData->AppDomain, true);
	}

	void RuntimeScriptEngine::ShutdownMono()
	{
		s_RuntimeData->AppDomain = nullptr;
		s_RuntimeData->RootDomain = nullptr;
	}

	MonoObject* RuntimeScriptEngine::InstantiateClass(MonoClass* monoClass)
	{
		MonoObject* instance = mono_object_new(s_RuntimeData->AppDomain, monoClass);
		mono_runtime_object_init(instance);
		return instance;
	}

	MonoImage* RuntimeScriptEngine::GetAssemblyImage()
	{
		return s_RuntimeData->CombinedAssemblyImage;
	}

	ScriptFieldRegistry& RuntimeScriptEngine::GetFieldRegistry()
	{
		return s_RuntimeData->FieldRegistry;
	}

	std::vector<Ref<ScriptInstance>>* RuntimeScriptEngine::GetEntityScriptInstances(UUID entityID)
	{
		auto it = s_RuntimeData->EntityInstances.find(entityID);
		if (it != s_RuntimeData->EntityInstances.end())
			return &it->second;
		return nullptr;
	}

	bool RuntimeScriptEngine::GetFieldValue(MonoObject* instance, const ScriptField& field, ScriptFieldValue& outValue)
	{
		if (!instance)
			return false;

		MonoClass* monoClass = mono_object_get_class(instance);
		MonoClassField* monoField = mono_class_get_field_from_name(monoClass, field.Name.c_str());
		if (!monoField)
			return false;

		return ScriptFieldHandler::GetField(instance, monoField, field.Type, outValue);
	}

	bool RuntimeScriptEngine::SetFieldValue(MonoObject* instance, const ScriptField& field, const ScriptFieldValue& value)
	{
		if (!instance)
			return false;

		MonoClass* monoClass = mono_object_get_class(instance);
		MonoClassField* monoField = mono_class_get_field_from_name(monoClass, field.Name.c_str());
		if (!monoField)
			return false;

		return ScriptFieldHandler::SetField(instance, monoField, field.Type, value);
	}

}

*/
