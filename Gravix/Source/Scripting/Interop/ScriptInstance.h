#pragma once

#include <string>

namespace Gravix 
{

	class ScriptInstance
	{
	public:
		ScriptInstance(const std::string& typeName)
		{
			typedef void* (*CreateScriptFn)(const char*);
			static auto createScript = (CreateScriptFn)ScriptEngine::GetFunction(
				"GravixEngine.Interop.ScriptInstanceManager, GravixScripting",
				"CreateScript"
			);

			if (createScript)
			{
				m_Instance = createScript((typeName + ", GravixScripting").c_str());
				m_TypeName = typeName;
			}
		}

		~ScriptInstance()
		{
			typedef void (*DestroyScriptFn)(void*);
			static auto destroyScript = (DestroyScriptFn)ScriptEngine::GetFunction(
				"GravixEngine.Interop.ScriptInstanceManager, GravixScripting",
				"DestroyScript"
			);
			if (destroyScript && m_Instance)
			{
				destroyScript(m_Instance);
				m_Instance = nullptr;
			}
		}
	private:
		void* m_Instance = nullptr;
		std::string m_TypeName;
	};

}