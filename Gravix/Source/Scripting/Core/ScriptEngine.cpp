#include "pch.h"
#include "ScriptEngine.h"
#include "Scripting/Editor/EditorScriptEngine.h"
#include "Scripting/Fields/ScriptFieldRegistry.h"

#include <mono/metadata/object.h>
#include <mono/metadata/class.h>

namespace Gravix
{

	// ScriptEngine now delegates to EditorScriptEngine
	// When PaK builder is ready, this will switch between EditorScriptEngine and RuntimeScriptEngine

	void ScriptEngine::Initialize()
	{
		EditorScriptEngine::Initialize();
	}

	void ScriptEngine::Shutdown()
	{
		EditorScriptEngine::Shutdown();
	}

	void ScriptEngine::OnRuntimeStart(Scene* scene)
	{
		EditorScriptEngine::OnRuntimeStart(scene);
	}

	void ScriptEngine::OnRuntimeStop()
	{
		EditorScriptEngine::OnRuntimeStop();
	}

	void ScriptEngine::OnCreateEntity(Entity entity)
	{
		EditorScriptEngine::OnCreateEntity(entity);
	}

	void ScriptEngine::OnUpdateEntity(Entity entity, float deltaTime)
	{
		EditorScriptEngine::OnUpdateEntity(entity, deltaTime);
	}

	Scene* ScriptEngine::GetSceneContext()
	{
		return EditorScriptEngine::GetSceneContext();
	}

	std::unordered_map<std::string, Ref<ScriptClass>>& ScriptEngine::GetEntityClasses()
	{
		return EditorScriptEngine::GetEntityClasses();
	}

	bool ScriptEngine::IsEntityClassExists(const std::string& fullClassName)
	{
		return EditorScriptEngine::IsEntityClassExists(fullClassName);
	}

	ScriptFieldRegistry& ScriptEngine::GetFieldRegistry()
	{
		return EditorScriptEngine::GetFieldRegistry();
	}

	std::vector<Ref<ScriptInstance>>* ScriptEngine::GetEntityScriptInstances(UUID entityID)
	{
		return EditorScriptEngine::GetEntityScriptInstances(entityID);
	}

	bool ScriptEngine::GetFieldValue(MonoObject* instance, const ScriptField& field, ScriptFieldValue& outValue)
	{
		return EditorScriptEngine::GetFieldValue(instance, field, outValue);
	}

	bool ScriptEngine::SetFieldValue(MonoObject* instance, const ScriptField& field, const ScriptFieldValue& value)
	{
		return EditorScriptEngine::SetFieldValue(instance, field, value);
	}

	MonoImage* ScriptEngine::GetCoreAssemblyImage()
	{
		return EditorScriptEngine::GetCoreAssemblyImage();
	}

	MonoImage* ScriptEngine::GetAppAssemblyImage()
	{
		return EditorScriptEngine::GetAppAssemblyImage();
	}

	MonoObject* ScriptEngine::InstantiateClass(MonoClass* monoClass)
	{
		return EditorScriptEngine::InstantiateClass(monoClass);
	}

	// ScriptClass implementation
	ScriptClass::ScriptClass(const std::string& classNamespace, const std::string& className)
		: m_ClassNamespace(classNamespace), m_ClassName(className)
	{
		if(m_ClassNamespace == "GravixEngine")
			m_MonoClass = mono_class_from_name(ScriptEngine::GetCoreAssemblyImage(), classNamespace.c_str(), className.c_str());
		else
			m_MonoClass = mono_class_from_name(ScriptEngine::GetAppAssemblyImage(), classNamespace.c_str(), className.c_str());
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

	const ScriptField* ScriptClass::GetField(const std::string& name) const
	{
		auto it = m_Fields.find(name);
		if (it == m_Fields.end())
			return nullptr;
		return &it->second;
	}

	// ScriptInstance implementation
	ScriptInstance::ScriptInstance(Ref<ScriptClass> scriptClass, Entity entity)
		: m_ScriptClass(scriptClass)
	{
		m_Instance = scriptClass->Instantiate();

		// Get Entity class from ScriptEngine
		auto& entityClasses = ScriptEngine::GetEntityClasses();
		Ref<ScriptClass> entityClass = CreateRef<ScriptClass>("GravixEngine", "Entity");

		m_Constructor = entityClass->GetMethod(".ctor", 1);
		m_OnCreateMethod = m_ScriptClass->GetMethod("OnCreate", 0);
		m_OnUpdateMethod = m_ScriptClass->GetMethod("OnUpdate", 1);

		// Call the constructor
		UUID entityID = entity.GetID();
		void* params = &entityID;

		scriptClass->InvokeMethod(m_Instance, m_Constructor, &params);

		// Apply stored field values from registry
		std::string fullClassName = scriptClass->GetFullClassName();
		auto& fieldRegistry = ScriptEngine::GetFieldRegistry();
		auto* scriptData = fieldRegistry.GetScriptInstanceData(entityID, fullClassName);
		if (scriptData)
		{
			for (const auto& [fieldName, field] : scriptClass->GetFields())
			{
				auto* storedValue = fieldRegistry.GetFieldValue(entityID, fullClassName, fieldName);
				if (storedValue)
				{
					ScriptEngine::SetFieldValue(m_Instance, field, *storedValue);
				}
			}
		}
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