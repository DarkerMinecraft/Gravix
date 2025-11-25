#pragma once

#include <filesystem>

namespace Gravix 
{

	class ScriptEngine 
	{
	public:
		static void Initialize(const std::filesystem::path& runtimeLibraryPath);
	private:
		static bool LoadHostFxr();
		static bool LoadRuntime(const std::filesystem::path& runtimeConfigPath);
	};

}