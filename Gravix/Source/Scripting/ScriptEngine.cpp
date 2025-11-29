#include "pch.h"
#include "ScriptEngine.h"

#include "ScriptGlue.h"

#include "Project/Project.h"

#include <fstream>
#include <filesystem>

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>

namespace Utils 
{
	static char* ReadBytes(const std::filesystem::path& filepath, uint32_t* outSize)
	{
		std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

		if (!stream)
		{
			// Failed to open the file
			return nullptr;
		}

		std::streampos end = stream.tellg();
		stream.seekg(0, std::ios::beg);
		uint32_t size = end - stream.tellg();

		if (size == 0)
		{
			// File is empty
			return nullptr;
		}

		char* buffer = new char[size];
		stream.read((char*)buffer, size);
		stream.close();

		*outSize = size;
		return buffer;
	}

	static MonoAssembly* LoadMonoAssembly(const std::filesystem::path& assemblyPath)
	{
		uint32_t fileSize = 0;
		char* fileData = ReadBytes(assemblyPath.string(), &fileSize);

		// NOTE: We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
		MonoImageOpenStatus status;
		MonoImage* image = mono_image_open_from_data_full(fileData, fileSize, 1, &status, 0);

		if (status != MONO_IMAGE_OK)
		{
			const char* errorMessage = mono_image_strerror(status);
			GX_CORE_ERROR("Failed to open C# assembly image from data: {0}", errorMessage);

			return nullptr;
		}

		MonoAssembly* assembly = mono_assembly_load_from_full(image, assemblyPath.string().c_str(), &status, 0);
		mono_image_close(image);

		// Don't forget to free the file data
		delete[] fileData;

		return assembly;
	}

	static void PrintAssemblyTypes(MonoAssembly* assembly)
	{
		MonoImage* image = mono_assembly_get_image(assembly);
		const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
		int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

		for (int32_t i = 0; i < numTypes; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

			const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
			const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

			MonoClass* monoClass = mono_class_from_name(image, nameSpace, name);
			MonoClass* entityClass = mono_class_from_name(image, "Gravix", "Entity");

			bool isEntity = mono_class_is_subclass_of(monoClass, entityClass, false);

			GX_CORE_TRACE("{0}.{1}", nameSpace, name);
		}
	}
}

namespace Gravix 
{

	struct ScriptEngineData
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
	};

	static Ref<ScriptEngineData> s_Data = nullptr;

	void ScriptEngine::Initialize()
	{
		s_Data = CreateRef<ScriptEngineData>();

		InitMono();
		LoadCoreAssembly(Project::GetScriptPath() / "bin/GravixScripting.dll");
		ScriptGlue::RegisterFunctions();
		LoadAppAssembly(Project::GetScriptPath() / "bin/OrbitPlayer.dll");
	}

	void ScriptEngine::Shutdown()
	{
		ShudownMono();
		s_Data = nullptr; // Automatically cleaned up by Ref<>
	}

	void ScriptEngine::LoadCoreAssembly(const std::filesystem::path& coreAssemblyPath)
	{
		s_Data->CoreAssembly = Utils::LoadMonoAssembly(coreAssemblyPath);
		s_Data->CoreAssemblyImage = mono_assembly_get_image(s_Data->CoreAssembly);

		s_Data->EntityClass = CreateRef<ScriptClass>("GravixEngine", "Entity");
	}

	void ScriptEngine::LoadAppAssembly(const std::filesystem::path& appAssemblyPath)
	{
		s_Data->AppAssembly = Utils::LoadMonoAssembly(appAssemblyPath);
		s_Data->AppAssemblyImage = mono_assembly_get_image(s_Data->AppAssembly);

		LoadAssemblyClasses(s_Data->AppAssemblyImage);
	}

	void ScriptEngine::OnRuntimeStart(Scene* scene)
	{
		s_Data->SceneContext = scene;
	}

	void ScriptEngine::OnRuntimeStop()
	{
		s_Data->EntityInstances.clear();
		s_Data->SceneContext = nullptr;
	}

	void ScriptEngine::OnCreateEntity(Entity entity)
	{
		UUID entityID = entity.GetID();
		s_Data->EntityInstances[entityID].clear();

		// Get all script components (supports multiple)
		auto scriptComponents = entity.GetComponents<ScriptComponent>();
		GX_CORE_INFO("OnCreateEntity called for entity '{0}' with {1} script component(s)", entity.GetName(), scriptComponents.size());

		for (auto* sc : scriptComponents)
		{
			GX_CORE_INFO("  Script component: {0}", sc->Name);
			if (ScriptEngine::IsEntityClassExists(sc->Name))
			{
				GX_CORE_INFO("  Creating script instance for: {0}", sc->Name);
				Ref<ScriptInstance> instance = CreateRef<ScriptInstance>(s_Data->EntityClasses[sc->Name], entity);
				s_Data->EntityInstances[entityID].push_back(instance);
				instance->InvokeOnCreate();
				GX_CORE_INFO("  OnCreate invoked for: {0}", sc->Name);
			}
			else
			{
				GX_CORE_WARN("  Script class does not exist: {0}", sc->Name);
			}
		}
	}

	void ScriptEngine::OnUpdateEntity(Entity entity, float deltaTime)
	{
		UUID uuid = entity.GetID();

		// Update all script instances for this entity
		if (s_Data->EntityInstances.find(uuid) != s_Data->EntityInstances.end())
		{
			for (auto& instance : s_Data->EntityInstances[uuid])
			{
				instance->InvokeOnUpdate(deltaTime);
			}
		}
	}

	Scene* ScriptEngine::GetSceneContext()
	{
		return s_Data->SceneContext;
	}

	std::unordered_map<std::string, Ref<ScriptClass>>& ScriptEngine::GetEntityClasses()
	{
		return s_Data->EntityClasses;
	}

	bool ScriptEngine::IsEntityClassExists(const std::string& fullClassName)
	{
		return s_Data->EntityClasses.contains(fullClassName);
	}

	void ScriptEngine::InitMono()
	{
		mono_set_assemblies_path("lib/mono/4.5");

		MonoDomain* domain = mono_jit_init("GravixJITRuntime");
		if (domain == nullptr)
		{
			GX_VERIFY("Failed to initialize Mono JIT runtime!");
		}

		s_Data->RootDomain = domain;

		s_Data->AppDomain = mono_domain_create_appdomain((char*)"GravixScriptRuntime", nullptr);
		mono_domain_set(s_Data->AppDomain, true);
	}

	void ScriptEngine::ShudownMono()
	{
		s_Data->AppDomain = nullptr;
		s_Data->RootDomain = nullptr;
	}

	void ScriptEngine::LoadAssemblyClasses(MonoImage* assemblyImage)
	{
		s_Data->EntityClasses.clear();

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
			MonoClass* entityClass = mono_class_from_name(s_Data->CoreAssemblyImage, "GravixEngine", "Entity");

			if (monoClass == entityClass)
				continue;

			bool isEntity = mono_class_is_subclass_of(monoClass, entityClass, false);
			if (!isEntity)
				continue;

			s_Data->EntityClasses[fullName] = CreateRef<ScriptClass>(nameSpace, name);
			
			int fieldCount = mono_class_num_fields(monoClass);

			GX_CORE_INFO("Loaded Script Entity Class: {0} with {1} fields", fullName, fieldCount);
			void* iterator = nullptr;
			while(MonoClassField* field = mono_class_get_fields(monoClass, &iterator))
			{
				const char* name = mono_field_get_name(field);
				GX_CORE_INFO("    Field: {0}", name);
			}
		}
	}

	MonoObject* ScriptEngine::InstantiateClass(MonoClass* monoClass)
	{
		MonoObject* instance = mono_object_new(s_Data->AppDomain, monoClass);
		mono_runtime_object_init(instance);
		return instance;
	}

	MonoImage* ScriptEngine::GetCoreAssemblyImage()
	{
		return s_Data->CoreAssemblyImage;
	}

	ScriptClass::ScriptClass(const std::string& classNamespace, const std::string& className)
		: m_ClassNamespace(classNamespace), m_ClassName(className)
	{
		if(m_ClassNamespace == "GravixEngine")
			m_MonoClass = mono_class_from_name(s_Data->CoreAssemblyImage, classNamespace.c_str(), className.c_str());
		else 
			m_MonoClass = mono_class_from_name(s_Data->AppAssemblyImage, classNamespace.c_str(), className.c_str());
	}

	MonoObject* ScriptClass::Instantiate()
	{
		return ScriptEngine::InstantiateClass(m_MonoClass);
	}

	MonoMethod* ScriptClass::GetMethod(const std::string& name, int parameterCount)
	{
		return mono_class_get_method_from_name(m_MonoClass, name.c_str(), parameterCount);
	}

	MonoObject* ScriptClass::InvokeMethod(MonoObject* instance, MonoMethod* method, void** params /*= nullptr*/)
	{
		return mono_runtime_invoke(method, instance, params, nullptr);
	}

	ScriptInstance::ScriptInstance(Ref<ScriptClass> scriptClass, Entity entity)
		: m_ScriptClass(scriptClass)
	{
		m_Instance = scriptClass->Instantiate();

		m_Constructor = s_Data->EntityClass->GetMethod(".ctor", 1);
		m_OnCreateMethod = m_ScriptClass->GetMethod("OnCreate", 0);
		m_OnUpdateMethod = m_ScriptClass->GetMethod("OnUpdate", 1);

		// Call the constructor
		UUID entityID = entity.GetID();
		void* params = &entityID;

		scriptClass->InvokeMethod(m_Instance, m_Constructor, &params);
	}

	void ScriptInstance::InvokeOnCreate()
	{
		if(m_OnCreateMethod)
			m_ScriptClass->InvokeMethod(m_Instance, m_OnCreateMethod);
	}

	void ScriptInstance::InvokeOnUpdate(float deltaTime)
	{
		void* params = &deltaTime;
		if(m_OnUpdateMethod)
			m_ScriptClass->InvokeMethod(m_Instance, m_OnUpdateMethod, &params);
	}

}