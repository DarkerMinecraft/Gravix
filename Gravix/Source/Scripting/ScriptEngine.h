#pragma once

#include <string>
#include <filesystem>
#include <functional>

namespace Gravix
{
	// Forward declaration
	class ScriptInstance;

	class ScriptEngine
	{
	public:
		static void Init(const std::filesystem::path& assemblyPath);
		static void Shutdown();

		static bool ReloadAssembly();

		static void* GetFunction(const std::string& typeName, const std::string& methodName);

		// Templated Call method - handles static function calls with variable arguments
		template<typename ReturnType, typename... Args>
		static ReturnType Call(const std::string& typeName, const std::string& methodName, Args... args)
		{
			typedef ReturnType(*FunctionPtr)(Args...);
			static auto function = (FunctionPtr)GetFunction(typeName, methodName);

			if (!function)
			{
				GX_CORE_ERROR("[ScriptEngine] Failed to get function: {}::{}", typeName, methodName);
				if constexpr (!std::is_void_v<ReturnType>)
					return ReturnType{};
				else
					return;
			}

			if constexpr (std::is_void_v<ReturnType>)
			{
				function(args...);
			}
			else
			{
				return function(args...);
			}
		}

		// Create an instance of a C# class
		static ScriptInstance CreateInstance(const std::string& typeName);

	private:
		static void InitDotNet(const std::filesystem::path& assemblyPath);
		static void ShutdownDotNet();

		static bool LoadAssembly(const std::filesystem::path& assemblyPath);

		static bool LoadHostFxr();
		static bool InitializeHostFxrContext(const std::filesystem::path& assemblyPath);
		static bool LoadAssemblyAndGetFunctionPointer();
	};
}