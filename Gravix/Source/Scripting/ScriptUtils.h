#pragma once

#include <filesystem>

extern "C"
{
	typedef struct _MonoAssembly MonoAssembly;
	typedef struct _MonoImage MonoImage;
}

namespace Gravix
{

	class ScriptUtils
	{
	public:
		static char* ReadBytes(const std::filesystem::path& filepath, uint32_t* outSize);
		static MonoAssembly* LoadMonoAssembly(const std::filesystem::path& assemblyPath);
		static void PrintAssemblyTypes(MonoAssembly* assembly);
	};

}
