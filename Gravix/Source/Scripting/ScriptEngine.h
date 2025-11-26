#pragma once

#include <filesystem>
#include <string>

#include "Core/RefCounted.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"

extern "C" 
{
	typedef struct _MonoClass MonoClass;
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoMethod MonoMethod;
	typedef struct _MonoImage MonoImage;
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
	private:
		MonoClass* m_MonoClass = nullptr;
		std::string m_ClassNamespace;
		std::string m_ClassName;
	};

	class ScriptEngine 
	{
	public:
		static void Initialize();
		static void Shutdown();

		static void LoadAssembly(const std::filesystem::path& assemblyPath);

		static void OnRuntimeStart(Scene* scene);
		static void OnRuntimeStop();

		static void OnCreateEntity(Entity entity);
		static void OnUpdateEntity(Entity entity, float deltaTime);

		static Scene* GetSceneContext();

		static std::unordered_map<std::string, Ref<ScriptClass>>& GetEntityClasses();
		static bool IsEntityClassExists(const std::string& fullClassName);
	private:
		static void InitMono();
		static void ShudownMono();

		static void LoadAssemblyClasses(MonoImage* image);

		static MonoObject* InstantiateClass(MonoClass* monoClass);

		friend class ScriptClass;
	};

	class ScriptInstance : public RefCounted
	{
	public:
		ScriptInstance(Ref<ScriptClass> scriptClass, Entity entity);

		void InvokeOnCreate();
		void InvokeOnUpdate(float deltaTime);
	private:
		Ref<ScriptClass> m_ScriptClass;

		MonoObject* m_Instance = nullptr;
		MonoMethod* m_Constructor = nullptr;
		MonoMethod* m_OnCreateMethod = nullptr;
		MonoMethod* m_OnUpdateMethod = nullptr;
	};

}