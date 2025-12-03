#include "pch.h"
#include "EditorScriptEngine.h"
#include "Scripting/Interop/ScriptGlue.h"
#include "Scripting/Interop/ScriptUtils.h"
#include "Scripting/Core/ScriptTypes.h"
#include "Scripting/Fields/ScriptFieldRegistry.h"
#include "Scripting/Fields/ScriptFieldHandler.h"

#include "Core/Console.h"
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

		// Load app assembly with project name
		std::string projectName = Project::GetActive()->GetConfig().Name;
		LoadAppAssembly(Project::GetScriptPath() / ("bin/" + projectName + ".dll"));
	}

	void EditorScriptEngine::Shutdown()
	{
		ShutdownMono();
		s_EditorData = nullptr;
	}

	void EditorScriptEngine::LoadCoreAssembly(const std::filesystem::path& coreAssemblyPath)
	{
		if (!std::filesystem::exists(coreAssemblyPath))
		{
			GX_CORE_ERROR("Core assembly not found at: {0}", coreAssemblyPath.string());
			return;
		}

		GX_CORE_INFO("Loading core assembly from: {0}", coreAssemblyPath.string());
		s_EditorData->CoreAssembly = ScriptUtils::LoadMonoAssembly(coreAssemblyPath);

		if (!s_EditorData->CoreAssembly)
		{
			GX_CORE_ERROR("Failed to load core assembly: {0}", coreAssemblyPath.string());
			return;
		}

		s_EditorData->CoreAssemblyImage = mono_assembly_get_image(s_EditorData->CoreAssembly);

		if (!s_EditorData->CoreAssemblyImage)
		{
			GX_CORE_ERROR("Failed to get image from core assembly");
			return;
		}

		s_EditorData->EntityClass = CreateRef<ScriptClass>("GravixEngine", "Entity");
		GX_CORE_INFO("Core assembly loaded successfully");
	}

	void EditorScriptEngine::LoadAppAssembly(const std::filesystem::path& appAssemblyPath)
	{
		if (!std::filesystem::exists(appAssemblyPath))
		{
			GX_CORE_ERROR("App assembly not found at: {0}", appAssemblyPath.string());
			return;
		}

		GX_CORE_INFO("Loading app assembly from: {0}", appAssemblyPath.string());
		s_EditorData->AppAssembly = ScriptUtils::LoadMonoAssembly(appAssemblyPath);

		if (!s_EditorData->AppAssembly)
		{
			GX_CORE_ERROR("Failed to load app assembly: {0}", appAssemblyPath.string());
			return;
		}

		s_EditorData->AppAssemblyImage = mono_assembly_get_image(s_EditorData->AppAssembly);

		if (!s_EditorData->AppAssemblyImage)
		{
			GX_CORE_ERROR("Failed to get image from app assembly");
			return;
		}

		LoadAssemblyClasses(s_EditorData->AppAssemblyImage);
		GX_CORE_INFO("App assembly loaded successfully");
	}

	void EditorScriptEngine::OnRuntimeStart(Scene* scene)
	{
		GX_CORE_INFO("EditorScriptEngine::OnRuntimeStart - Entering play mode");
		s_EditorData->SceneContext = scene;
	}

	void EditorScriptEngine::OnRuntimeStop()
	{
		GX_CORE_INFO("EditorScriptEngine::OnRuntimeStop - Exiting play mode");
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

	// Hot Reload Implementation
	static Scope<ScriptFileWatcher> s_ScriptWatcher = nullptr;

	void EditorScriptEngine::ReloadAppAssembly()
	{
		GX_CORE_INFO("=== Reloading App Assembly ===");

		// Check if script engine is initialized
		if (!s_EditorData)
		{
			GX_CORE_ERROR("Cannot reload assembly: Script engine not initialized");
			return;
		}

		// Only reload when not in play mode
		if (s_EditorData->SceneContext != nullptr)
		{
			GX_CORE_WARN("Cannot reload scripts during play mode");
			return;
		}

		try
		{
			// Build C# project to detect syntax errors
			GX_CORE_INFO("Building C# project...");

			// Use project name for .csproj file
			std::string projectName = Project::GetActive()->GetConfig().Name;
			std::filesystem::path csprojPath = Project::GetScriptPath() / (projectName + ".csproj");

			if (!std::filesystem::exists(csprojPath))
			{
				Console::LogError("C# project file not found: " + csprojPath.string());
				GX_CORE_ERROR("C# project file not found: {0}", csprojPath.string());
				return;
			}

			// Run dotnet build and capture output
			std::string buildCommand = "dotnet build \"" + csprojPath.string() + "\" --nologo 2>&1";
			FILE* pipe = _popen(buildCommand.c_str(), "r");

			if (!pipe)
			{
				Console::LogError("Failed to run dotnet build. Is .NET SDK installed?");
				GX_CORE_ERROR("Failed to run dotnet build");
				return;
			}

			// Read build output
			char buffer[256];
			std::string buildOutput;
			bool hasErrors = false;
			int errorCount = 0;

			while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
			{
				std::string line(buffer);
				buildOutput += line;

				// Remove trailing newline
				if (!line.empty() && line.back() == '\n')
					line.pop_back();

				// Check for actual C# compilation errors (format: "file.cs(line,col): error CSXXXX: message")
				// Don't match summary lines like "Build failed with X error(s)"
				if (line.find(": error CS") != std::string::npos)
				{
					hasErrors = true;
					errorCount++;
					Console::LogError(line);
				}
				// Check for warnings (format: "file.cs(line,col): warning CSXXXX: message")
				else if (line.find(": warning CS") != std::string::npos)
				{
					Console::LogWarning(line);
				}
			}

			int buildResult = _pclose(pipe);

			if (buildResult != 0 || hasErrors)
			{
				std::string errorMsg = "C# build failed with " + std::to_string(errorCount) + " error(s). Fix errors to reload.";
				Console::LogError(errorMsg);
				GX_CORE_ERROR("Hot reload aborted: {0}", errorMsg);
				// Early return - do NOT reload assembly when there are errors
				return;
			}

			GX_CORE_INFO("C# build succeeded - proceeding with hot reload");

			// Store field registry before reload
			auto fieldRegistryCopy = s_EditorData->FieldRegistry;

			// Unload app domain
			if (s_EditorData->AppDomain)
			{
				GX_CORE_INFO("Unloading app domain...");
				mono_domain_set(mono_get_root_domain(), false);
				mono_domain_unload(s_EditorData->AppDomain);
				s_EditorData->AppDomain = nullptr;
			}

			// Create new app domain
			GX_CORE_INFO("Creating new app domain...");
			s_EditorData->AppDomain = mono_domain_create_appdomain((char*)"GravixAppDomain", nullptr);

			if (!s_EditorData->AppDomain)
			{
				GX_CORE_ERROR("Failed to create app domain");
				return;
			}

			mono_domain_set(s_EditorData->AppDomain, false);
			
			std::filesystem::path coreAssemblyPath = Project::GetScriptPath() / ("bin/GravixScripting.dll");
			if (!std::filesystem::exists(coreAssemblyPath))
			{
				GX_CORE_ERROR("Core assembly not found during hot reload: {0}", coreAssemblyPath.string());
				return;
			}
			// Reload core assembly
			GX_CORE_INFO("Reloading core assembly from: {0}", coreAssemblyPath.string());
			s_EditorData->CoreAssembly = ScriptUtils::LoadMonoAssembly(coreAssemblyPath);
			if (!s_EditorData->CoreAssembly)
			{
				GX_CORE_ERROR("Failed to reload core assembly");
				return;
			}
			s_EditorData->CoreAssemblyImage = mono_assembly_get_image(s_EditorData->CoreAssembly);

			// Reload app assembly
			std::filesystem::path appAssemblyPath = Project::GetScriptPath() / ("bin/" + projectName + ".dll");

			if (!std::filesystem::exists(appAssemblyPath))
			{
				GX_CORE_ERROR("App assembly not found: {0}", appAssemblyPath.string());
				return;
			}

			GX_CORE_INFO("Loading app assembly from: {0}", appAssemblyPath.string());
			s_EditorData->AppAssembly = ScriptUtils::LoadMonoAssembly(appAssemblyPath);

			if (!s_EditorData->AppAssembly)
			{
				GX_CORE_ERROR("Failed to load app assembly");
				return;
			}

			s_EditorData->AppAssemblyImage = mono_assembly_get_image(s_EditorData->AppAssembly);

			// Reload classes
			GX_CORE_INFO("Reloading classes...");
			LoadAssemblyClasses(s_EditorData->AppAssemblyImage);

			ScriptGlue::RegisterFunctions();

			// Restore field registry
			s_EditorData->FieldRegistry = fieldRegistryCopy;

			GX_CORE_INFO("App assembly reloaded successfully! ({0} classes)", s_EditorData->EntityClasses.size());
		}
		catch (const std::exception& e)
		{
			Console::LogError("Hot reload failed: " + std::string(e.what()));
			GX_CORE_ERROR("Exception during hot reload: {0}", e.what());
		}
		catch (...)
		{
			Console::LogError("Hot reload failed: Unknown exception");
			GX_CORE_ERROR("Unknown exception during hot reload");
		}
	}

	void EditorScriptEngine::StartWatchingScripts(const std::filesystem::path& scriptPath)
	{
		if (!s_ScriptWatcher)
			s_ScriptWatcher = CreateScope<ScriptFileWatcher>();

		s_ScriptWatcher->StartWatching(scriptPath);
		GX_CORE_INFO("Script file watcher started");
	}

	void EditorScriptEngine::StopWatchingScripts()
	{
		if (s_ScriptWatcher)
		{
			s_ScriptWatcher->StopWatching();
			s_ScriptWatcher.reset();
		}
	}

	void EditorScriptEngine::CheckForScriptReload()
	{
		if (!s_ScriptWatcher || !s_EditorData)
		{
			GX_CORE_WARN("CheckForScriptReload: Watcher or EditorData is null");
			return;
		}

		// Check for file changes (polling-based)
		s_ScriptWatcher->CheckForChanges();

		// Check if we're in play mode (SceneContext is set during runtime)
		bool isInPlayMode = (s_EditorData->SceneContext != nullptr);

		if (isInPlayMode)
		{
			// Hot reload is disabled during play mode to prevent crashes
			// User needs to stop play mode before scripts can be reloaded
			static bool hasWarned = false;
			if (s_ScriptWatcher->ShouldReload() && !hasWarned)
			{
				GX_CORE_WARN("Script changes detected but hot reload is disabled during play mode. Stop play mode to reload.");
				hasWarned = true;
			}
			return;
		}

		if (s_ScriptWatcher->ShouldReload())
		{
			// Check if enough time has passed since the last change (debounce)
			// Wait for 500ms of "quiet time" with no changes before reloading
			int64_t millisecondsSinceChange = s_ScriptWatcher->GetMillisecondsSinceLastChange();

			GX_CORE_INFO("CheckForScriptReload: Reload pending, time since change: {}ms", millisecondsSinceChange);

			if (millisecondsSinceChange >= 500)
			{
				s_ScriptWatcher->ClearReloadFlag();

				GX_CORE_INFO("Script changes detected - triggering hot reload...");

				// Give a small delay for file operations to complete
				std::this_thread::sleep_for(std::chrono::milliseconds(200));

				ReloadAppAssembly();
			}
		}
	}

}
