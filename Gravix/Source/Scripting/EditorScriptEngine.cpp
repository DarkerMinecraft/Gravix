#include "pch.h"
#include "EditorScriptEngine.h"
#include "ScriptGlue.h"
#include "ScriptUtils.h"
#include "ScriptTypes.h"
#include "ScriptFieldRegistry.h"
#include "ScriptFieldHandler.h"

#include "Project/Project.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/class.h>
#include <mono/metadata/attrdefs.h>

#include <glm/glm.hpp>

namespace Gravix
{

	struct EditorScriptEngineData
	{
		MonoDomain* RootDomain = nullptr;
		MonoDomain* AppDomain = nullptr;

		MonoAssembly* CoreAssembly = nullptr;
		MonoImage* CoreAssemblyImage = nullptr;

		MonoAssembly* AppAssembly = nullptr;
		MonoImage* AppAssemblyImage = nullptr;

		Scene* SceneContext = nullptr;

		Ref<ScriptClass> EntityClass;

		std::unordered_map<std::string, Ref<ScriptClass>> EntityClasses;
		std::unordered_map<UUID, std::vector<Ref<ScriptInstance>>> EntityInstances;

		ScriptFieldRegistry FieldRegistry;
	};

	static Ref<EditorScriptEngineData> s_EditorData = nullptr;

	void EditorScriptEngine::Initialize()
	{
		s_EditorData = CreateRef<EditorScriptEngineData>();

		InitMono();
		LoadCoreAssembly(Project::GetScriptPath() / "bin/GravixScripting.dll");
		ScriptGlue::RegisterFunctions();
		LoadAppAssembly(Project::GetScriptPath() / "bin/OrbitPlayer.dll");
	}

	void EditorScriptEngine::Shutdown()
	{
		ShutdownMono();
		s_EditorData = nullptr;
	}

	void EditorScriptEngine::LoadCoreAssembly(const std::filesystem::path& coreAssemblyPath)
	{
		s_EditorData->CoreAssembly = ScriptUtils::LoadMonoAssembly(coreAssemblyPath);
		s_EditorData->CoreAssemblyImage = mono_assembly_get_image(s_EditorData->CoreAssembly);

		s_EditorData->EntityClass = CreateRef<ScriptClass>("GravixEngine", "Entity");
	}

	void EditorScriptEngine::LoadAppAssembly(const std::filesystem::path& appAssemblyPath)
	{
		s_EditorData->AppAssembly = ScriptUtils::LoadMonoAssembly(appAssemblyPath);
		s_EditorData->AppAssemblyImage = mono_assembly_get_image(s_EditorData->AppAssembly);

		LoadAssemblyClasses(s_EditorData->AppAssemblyImage);
	}

	void EditorScriptEngine::OnRuntimeStart(Scene* scene)
	{
		s_EditorData->SceneContext = scene;
	}

	void EditorScriptEngine::OnRuntimeStop()
	{
		s_EditorData->EntityInstances.clear();
		s_EditorData->SceneContext = nullptr;
	}

	void EditorScriptEngine::OnCreateEntity(Entity entity)
	{
		UUID entityID = entity.GetID();
		auto& instances = s_EditorData->EntityInstances[entityID];
		instances.clear();

		auto scriptComponents = entity.GetComponents<ScriptComponent>();
		GX_CORE_INFO("OnCreateEntity: '{0}' has {1} script(s)", entity.GetName(), scriptComponents.size());

		for (auto* scriptComponent : scriptComponents)
		{
			if (!IsEntityClassExists(scriptComponent->Name))
			{
				GX_CORE_WARN("Script class not found: {0}", scriptComponent->Name);
				continue;
			}

			auto scriptClass = s_EditorData->EntityClasses[scriptComponent->Name];
			auto instance = CreateRef<ScriptInstance>(scriptClass, entity);
			instances.push_back(instance);
			instance->InvokeOnCreate();

			GX_CORE_INFO("Initialized script: {0}", scriptComponent->Name);
		}
	}

	void EditorScriptEngine::OnUpdateEntity(Entity entity, float deltaTime)
	{
		UUID entityID = entity.GetID();

		auto it = s_EditorData->EntityInstances.find(entityID);
		if (it == s_EditorData->EntityInstances.end())
			return;

		for (auto& instance : it->second)
			instance->InvokeOnUpdate(deltaTime);
	}

	void EditorScriptEngine::OnDestroyEntity(Entity entity)
	{
		UUID entityID = entity.GetID();

		// Erase the entity from the map to prevent memory leaks
		s_EditorData->EntityInstances.erase(entityID);
	}

	Scene* EditorScriptEngine::GetSceneContext()
	{
		return s_EditorData->SceneContext;
	}

	std::unordered_map<std::string, Ref<ScriptClass>>& EditorScriptEngine::GetEntityClasses()
	{
		return s_EditorData->EntityClasses;
	}

	bool EditorScriptEngine::IsEntityClassExists(const std::string& fullClassName)
	{
		return s_EditorData->EntityClasses.contains(fullClassName);
	}

	void EditorScriptEngine::InitMono()
	{
		mono_set_assemblies_path("lib/mono/4.5");

		MonoDomain* domain = mono_jit_init("GravixJITRuntime");
		if (domain == nullptr)
		{
			GX_VERIFY("Failed to initialize Mono JIT runtime!");
		}

		s_EditorData->RootDomain = domain;

		s_EditorData->AppDomain = mono_domain_create_appdomain((char*)"GravixScriptRuntime", nullptr);
		mono_domain_set(s_EditorData->AppDomain, true);
	}

	void EditorScriptEngine::ShutdownMono()
	{
		s_EditorData->AppDomain = nullptr;
		s_EditorData->RootDomain = nullptr;
	}

	void EditorScriptEngine::LoadAssemblyClasses(MonoImage* assemblyImage)
	{
		s_EditorData->EntityClasses.clear();

		const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(assemblyImage, MONO_TABLE_TYPEDEF);
		int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

		for (int32_t i = 0; i < numTypes; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

			const char* nameSpace = mono_metadata_string_heap(assemblyImage, cols[MONO_TYPEDEF_NAMESPACE]);
			const char* name = mono_metadata_string_heap(assemblyImage, cols[MONO_TYPEDEF_NAME]);
			std::string fullName;
			if(strlen(nameSpace) != 0)
				fullName = fmt::format("{}.{}", nameSpace, name);
			else
				fullName = name;


			MonoClass* monoClass = mono_class_from_name(assemblyImage, nameSpace, name);
			MonoClass* entityClass = mono_class_from_name(s_EditorData->CoreAssemblyImage, "GravixEngine", "Entity");

			if (monoClass == entityClass)
				continue;

			bool isEntity = mono_class_is_subclass_of(monoClass, entityClass, false);
			if (!isEntity)
				continue;

			Ref<ScriptClass> scriptClass = CreateRef<ScriptClass>(nameSpace, name);
			s_EditorData->EntityClasses[fullName] = scriptClass;

			// Create a default instance to extract default field values
			MonoObject* defaultInstance = mono_object_new(s_EditorData->AppDomain, monoClass);
			mono_runtime_object_init(defaultInstance);

			// Extract public fields (and private fields with [SerializeField])
			void* iterator = nullptr;
			while (MonoClassField* field = mono_class_get_fields(monoClass, &iterator))
			{
				const char* fieldName = mono_field_get_name(field);
				uint32_t flags = mono_field_get_flags(field);

				// Check for [SerializeField] attribute on private/protected fields
				bool hasSerializeField = false;
				MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(monoClass, field);
				if (attrInfo)
				{
					for (int i = 0; i < attrInfo->num_attrs; i++)
					{
						MonoClass* attrClass = mono_method_get_class(attrInfo->attrs[i].ctor);
						const char* attrClassName = mono_class_get_name(attrClass);
						if (strcmp(attrClassName, "SerializeField") == 0)
						{
							hasSerializeField = true;
							break;
						}
					}
				}

				// Only process public fields or fields with [SerializeField]
				bool isPublic = (flags & MONO_FIELD_ATTR_PUBLIC);
				if (!isPublic && !hasSerializeField)
					continue;

				MonoType* type = mono_field_get_type(field);
				ScriptFieldType scriptType = ScriptTypeUtils::MonoTypeToScriptType(type);

				if (scriptType == ScriptFieldType::None)
					continue;

				int32_t alignment;
				uint32_t size = mono_type_size(type, &alignment);
				uint32_t offset = mono_field_get_offset(field);

				ScriptField scriptField(fieldName, scriptType, size, offset);
				scriptField.HasSerializeField = hasSerializeField;

				// Check for [Range] attribute
				if (attrInfo)
				{
					for (int i = 0; i < attrInfo->num_attrs; i++)
					{
						MonoClass* attrClass = mono_method_get_class(attrInfo->attrs[i].ctor);
						const char* attrClassName = mono_class_get_name(attrClass);
						if (strcmp(attrClassName, "Range") == 0)
						{
							// Get the constructor arguments (min, max)
							MonoObject* attrInstance = mono_custom_attrs_get_attr(attrInfo, attrClass);
							if (attrInstance)
							{
								// Get min and max fields from Range attribute
								MonoClassField* minField = mono_class_get_field_from_name(attrClass, "min");
								MonoClassField* maxField = mono_class_get_field_from_name(attrClass, "max");
								if (minField && maxField)
								{
									mono_field_get_value(attrInstance, minField, &scriptField.RangeMin);
									mono_field_get_value(attrInstance, maxField, &scriptField.RangeMax);
									scriptField.HasRange = true;
								}
							}
							break;
						}
					}
				}

				// Extract default value from the default instance
				ScriptFieldValue defaultValue;
				defaultValue.Type = scriptType;
				if (GetFieldValue(defaultInstance, scriptField, defaultValue))
				{
					scriptField.DefaultValue = defaultValue;
				}
				else
				{
					// Set to zero/false if couldn't read default
					memset(defaultValue.Data, 0, sizeof(defaultValue.Data));
					scriptField.DefaultValue = defaultValue;
				}

				scriptClass->m_Fields[fieldName] = scriptField;
			}

			int fieldCount = scriptClass->m_Fields.size();
			GX_CORE_INFO("Loaded Script Entity Class: {0} with {1} public fields", fullName, fieldCount);
		}
	}

	MonoObject* EditorScriptEngine::InstantiateClass(MonoClass* monoClass)
	{
		MonoObject* instance = mono_object_new(s_EditorData->AppDomain, monoClass);
		mono_runtime_object_init(instance);
		return instance;
	}

	MonoImage* EditorScriptEngine::GetCoreAssemblyImage()
	{
		return s_EditorData->CoreAssemblyImage;
	}

	MonoImage* EditorScriptEngine::GetAppAssemblyImage()
	{
		return s_EditorData->AppAssemblyImage;
	}

	ScriptFieldRegistry& EditorScriptEngine::GetFieldRegistry()
	{
		return s_EditorData->FieldRegistry;
	}

	std::vector<Ref<ScriptInstance>>* EditorScriptEngine::GetEntityScriptInstances(UUID entityID)
	{
		auto it = s_EditorData->EntityInstances.find(entityID);
		if (it != s_EditorData->EntityInstances.end())
			return &it->second;
		return nullptr;
	}

	bool EditorScriptEngine::GetFieldValue(MonoObject* instance, const ScriptField& field, ScriptFieldValue& outValue)
	{
		if (!instance)
			return false;

		MonoClass* monoClass = mono_object_get_class(instance);
		MonoClassField* monoField = mono_class_get_field_from_name(monoClass, field.Name.c_str());
		if (!monoField)
			return false;

		return ScriptFieldHandler::GetField(instance, monoField, field.Type, outValue);
	}

	bool EditorScriptEngine::SetFieldValue(MonoObject* instance, const ScriptField& field, const ScriptFieldValue& value)
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
