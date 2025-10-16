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

		static bool ReloadAssembly();

		static void* GetFunction(const std::string& typeName, const std::string& methodName);
	private:
		static void InitDotNet(const std::filesystem::path& assemblyPath);
		static void ShutdownDotNet();

		static bool LoadAssembly(const std::filesystem::path& assemblyPath);

		static bool LoadHostFxr();
		static bool InitializeHostFxrContext(const std::filesystem::path& assemblyPath);
		static bool LoadAssemblyAndGetFunctionPointer();
	};
}