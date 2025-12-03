#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include "Core/RefCounted.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scripting/Fields/ScriptField.h"

extern "C"
{
	typedef struct _MonoClass MonoClass;
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoMethod MonoMethod;
	typedef struct _MonoImage MonoImage;
	typedef struct _MonoClassField MonoClassField;
}

namespace Gravix
{

	class ScriptClass : public RefCounted
	{
	public:
		ScriptClass() = default;
		ScriptClass(const std::string& classNamespace, const std::string& className);

		MonoObject* Instantiate();
		MonoMethod* GetMethod(const std::string& name, int parameterCount);
		MonoObject* InvokeMethod(MonoObject* instance, MonoMethod* method, void** params = nullptr);

		const std::string& GetClassName() const { return m_ClassName; }
		std::string GetFullClassName() const
		{
			if (m_ClassNamespace.empty())
				return m_ClassName;
			return m_ClassNamespace + "." + m_ClassName;
		}
		const std::unordered_map<std::string, ScriptField>& GetFields() const { return m_Fields; }
		const ScriptField* GetField(const std::string& name) const;

	private:
		MonoClass* m_MonoClass = nullptr;
		std::string m_ClassNamespace;
		std::string m_ClassName;
		std::unordered_map<std::string, ScriptField> m_Fields;

		friend class ScriptEngine;
		friend class EditorScriptEngine;
		friend class RuntimeScriptEngine;
	};

	class ScriptFieldRegistry;
	class ScriptInstance;

	// Base ScriptEngine interface - delegates to EditorScriptEngine or RuntimeScriptEngine
	class ScriptEngine
	{
	public:
		// Currently only EditorScriptEngine is implemented
		// RuntimeScriptEngine will be enabled when PaK builder is ready
		static void Initialize();
		static void Shutdown();

		static void OnRuntimeStart(Scene* scene);
		static void OnRuntimeStop();

		static void OnCreateEntity(Entity entity);
		static void OnUpdateEntity(Entity entity, float deltaTime);

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

		// Internal access for ScriptClass
		static MonoImage* GetCoreAssemblyImage();
		static MonoImage* GetAppAssemblyImage();
		static MonoObject* InstantiateClass(MonoClass* monoClass);

	private:
		friend class ScriptClass;
		friend class ScriptGlue;
		friend class ScriptInstance;
	};

	class ScriptInstance : public RefCounted
	{
	public:
		ScriptInstance(Ref<ScriptClass> scriptClass, Entity entity);

		void InvokeOnCreate();
		void InvokeOnUpdate(float deltaTime);

		Ref<ScriptClass> GetScriptClass() const { return m_ScriptClass; }
		MonoObject* GetMonoObject() const { return m_Instance; }

	private:
		Ref<ScriptClass> m_ScriptClass;

		MonoObject* m_Instance = nullptr;
		MonoMethod* m_Constructor = nullptr;
		MonoMethod* m_OnCreateMethod = nullptr;
		MonoMethod* m_OnUpdateMethod = nullptr;
	};

}