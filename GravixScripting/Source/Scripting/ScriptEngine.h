#pragma once

#include <string>
#include <filesystem>

namespace Gravix
{
	class ScriptEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static bool LoadAssembly(const std::filesystem::path& assemblyPath);
		static bool ReloadAssembly();

		static void* GetFunction(const std::string& typeName, const std::string& methodName);
	private:
		static void InitDotNet();
		static void ShutdownDotNet();

		static bool LoadHostFxr();
		static bool InitializeHostFxrContext(const std::filesystem::path& assemblyPath);
		static bool LoadAssemblyAndGetFunctionPointer();
	};
}