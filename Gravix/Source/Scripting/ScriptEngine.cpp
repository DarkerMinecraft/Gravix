#include "pch.h"
#include "ScriptEngine.h"

#include <nethost.h>
#include <coreclr_delegates.h>
#include <hostfxr.h>

static void* LoadAndGetLibrary(const char_t* libraryPath) 
{
#ifdef ENGINE_PLATFORM_WINDOWS
	return LoadLibraryW(libraryPath);
#else 
	return dlOpen(libraryPath, RTLD_LAZY | RTLD_LOCAL);
#endif 
}

static void* GetExport(void* host, const char* name) 
{
#ifdef ENGINE_PLATFORM_WINDOWS
	return ::GetProcAddress((HMODULE)host, name);
#else
	return dlysm(host, name);
#endif
}

namespace Gravix 
{

	struct DotNetData 
	{
		void* HostModule;

		hostfxr_initialize_for_runtime_config_fn InitFuncPtr;
		hostfxr_get_runtime_delegate_fn GetDelegateFuncPtr;
		hostfxr_close_fn CloseFuncPtr;

		load_assembly_and_get_function_pointer_fn GetDotnetLoadAssemblyFuncPtr;
	};

	static DotNetData* s_Data;

	void ScriptEngine::Initialize(const std::filesystem::path& runtimeLibraryPath)
	{
		s_Data = new DotNetData();

		if (!LoadHostFxr())
			GX_CORE_ERROR("Failed to load HostFXR");
		if (!LoadRuntime(runtimeLibraryPath))
			GX_CORE_ERROR("Failed to load runtime");
	}

	bool ScriptEngine::LoadHostFxr()
	{
		char_t buffer[MAX_PATH];
		size_t bufferSize = sizeof(buffer) / sizeof(char_t);

		int32_t result = get_hostfxr_path(buffer, &bufferSize, nullptr);
		if (result != 0) 
		{
			GX_CORE_ERROR("Unable to find hostfxr (.NET Runtime)");
			return false;
		}

		s_Data->HostModule = LoadAndGetLibrary(buffer);

		if (!s_Data->HostModule) 
		{
			GX_CORE_ERROR("Unable to load hostfxr");
			return false;
		}

		s_Data->InitFuncPtr = (hostfxr_initialize_for_runtime_config_fn)GetExport(s_Data->HostModule, "hostfxr_initialize_for_runtime_config");
		s_Data->GetDelegateFuncPtr = (hostfxr_get_runtime_delegate_fn)GetExport(s_Data->HostModule, "hostfxr_get_runtime_delegate");
		s_Data->CloseFuncPtr = (hostfxr_close_fn)GetExport(s_Data->HostModule, "hostfxr_close");

		return (s_Data->InitFuncPtr && s_Data->GetDelegateFuncPtr && s_Data->CloseFuncPtr);
	}

	bool ScriptEngine::LoadRuntime(const std::filesystem::path& runtimeConfigPath)
	{
		hostfxr_handle cxt = nullptr;
		int rc = s_Data->InitFuncPtr(runtimeConfigPath.c_str(), nullptr, &cxt);
		if (rc != 0 || cxt == nullptr) 
		{
			GX_CORE_ERROR("Init Failed: {0}", rc);
			s_Data->CloseFuncPtr(cxt);
			return false;
		}

		void* load_assembly_and_get_function_pointer = nullptr;
		rc = s_Data->GetDelegateFuncPtr(
			cxt,
			hdt_load_assembly_and_get_function_pointer,
			&load_assembly_and_get_function_pointer);
		if (rc != 0 || load_assembly_and_get_function_pointer == nullptr) 
		{
			GX_CORE_ERROR("Get Delegate Failed: {0}", rc);
			s_Data->CloseFuncPtr(cxt);
			return false;
		}

		s_Data->GetDotnetLoadAssemblyFuncPtr = (load_assembly_and_get_function_pointer_fn)load_assembly_and_get_function_pointer;
		return true;
	}

}