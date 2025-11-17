#include "ScriptEngine.h"
#include "Interop/ScriptInstance.h"

#ifdef ENGINE_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include <nethost.h>
#include <coreclr_delegates.h>
#include <hostfxr.h>
#include <iostream>
#include <minwindef.h>
#include <codecvt>

#ifdef ENGINE_PLATFORM_WINDOWS
#define STR(s) L ## s
#define CH(c) L ## c
#define DIR_SEPARATOR L'\\'
#else
#define STR(s) s
#define CH(c) c
#define DIR_SEPARATOR '/'
#endif

// Define UNMANAGEDCALLERSONLY_METHOD if not already defined
#ifndef UNMANAGEDCALLERSONLY_METHOD
#define UNMANAGEDCALLERSONLY_METHOD ((const char_t*)-1)
#endif

namespace Gravix
{
	// --- Function Pointer Types ---
	using HostfxrInitializeForRuntimeConfigFn = int32_t(*)(
		const char_t* runtime_config_path,
		const struct hostfxr_initialize_parameters* parameters,
		hostfxr_handle* host_context_handle
		);
	using HostfxrGetRuntimeDelegateFn = int32_t(*)(
		const hostfxr_handle host_context_handle,
		enum hostfxr_delegate_type type,
		void** delegate
		);
	using HostfxrCloseFn = int32_t(*)(const hostfxr_handle host_context_handle);
	using HostfxrSetErrorWriterFn = hostfxr_error_writer_fn(*)(hostfxr_error_writer_fn error_writer);
	using HostfxrSetRuntimePropertyValueFn = int32_t(*)(
		const hostfxr_handle host_context_handle,
		const char_t* name,
		const char_t* value
		);

	// --- ScriptEngine Data ---
	struct ScriptEngineData
	{
		void* m_HostfxrLib = nullptr;
		hostfxr_handle m_HostContext = nullptr;

		HostfxrInitializeForRuntimeConfigFn m_InitFxrForRuntimeConfig = nullptr;
		HostfxrGetRuntimeDelegateFn m_GetRuntimeDelegate = nullptr;
		HostfxrCloseFn m_Close = nullptr;
		HostfxrSetErrorWriterFn m_SetErrorWriter = nullptr;
		HostfxrSetRuntimePropertyValueFn m_SetRuntimePropertyValue = nullptr;

		load_assembly_and_get_function_pointer_fn m_LoadAssemblyAndGetFunctionPointer = nullptr;

		std::filesystem::path m_AssemblyPath;
	};

	static ScriptEngineData s_Data;

	// --- Helper: Load Native Library ---
	static void* LoadNativeLibrary(const char_t* path)
	{
#ifdef ENGINE_PLATFORM_WINDOWS
		HMODULE h = ::LoadLibraryW(path);
		if (!h)
			GX_CORE_ERROR("[ScriptEngine] Failed to load library: {}", GetLastError());
		return (void*)h;
#else
		void* h = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
		if (!h)
			GX_CORE_ERROR("[ScriptEngine] Failed to load library: {}", dlerror());
		return h;
#endif
	}

	// --- Helper: Get Export from Library ---
	static void* GetExport(void* lib, const char* name)
	{
#ifdef ENGINE_PLATFORM_WINDOWS
		void* f = ::GetProcAddress((HMODULE)lib, name);
		if (!f)
			GX_CORE_ERROR("[ScriptEngine] Failed to get export: {}", name);
		return f;
#else
		void* f = dlsym(lib, name);
		if (!f)
			GX_CORE_ERROR("[ScriptEngine] Failed to get export: {}", name);
		return f;
#endif
	}

	// --- Error Writer Callback ---
	static void HostfxrErrorWriter(const char_t* message)
	{
#ifdef ENGINE_PLATFORM_WINDOWS
		std::wstring wstr(message);
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		std::string str = converter.to_bytes(wstr);
		GX_CORE_INFO("[HOSTFXR] {}", str);
#else
		GX_CORE_INFO("[HOSTFXR] {}", message);
#endif
	}

	// --- Public API ---
	void ScriptEngine::Init(const std::filesystem::path& assemblyPath)
	{
		GX_CORE_INFO("[ScriptEngine] Initializing Script Engine with assembly: {}", assemblyPath.string());		InitDotNet(assemblyPath);
	}

	void ScriptEngine::Shutdown()
	{
		GX_CORE_INFO("[ScriptEngine] Shutting down Script Engine");
		ShutdownDotNet();
	}

	bool ScriptEngine::LoadAssembly(const std::filesystem::path& assemblyPath)
	{
		if (!std::filesystem::exists(assemblyPath))
		{
			GX_CORE_ERROR("[ScriptEngine] Assembly not found: {}", assemblyPath.string());
			return false;
		}

		s_Data.m_AssemblyPath = assemblyPath;

		// Load hostfxr if not already loaded
		if (!s_Data.m_HostfxrLib && !LoadHostFxr())
		{
			GX_CORE_ERROR("[ScriptEngine] Failed to load hostfxr");
			return false;
		}

		// Initialize the host context for this assembly (self-contained mode)
		if (!InitializeHostFxrContext(assemblyPath))
		{
			GX_CORE_ERROR("[ScriptEngine] Failed to initialize hostfxr context");
			return false;
		}

		// Get the load_assembly_and_get_function_pointer delegate
		if (!LoadAssemblyAndGetFunctionPointer())
		{
			GX_CORE_ERROR("[ScriptEngine] Failed to get assembly loader delegate");
			return false;
		}

		GX_CORE_INFO("[ScriptEngine] Successfully loaded assembly: {}", assemblyPath.string());
		return true;
	}

	bool ScriptEngine::ReloadAssembly()
	{
		if (s_Data.m_AssemblyPath.empty())
		{
			GX_CORE_ERROR("[ScriptEngine] No assembly loaded to reload");
			return false;
		}

		// Close current context
		if (s_Data.m_HostContext && s_Data.m_Close)
		{
			s_Data.m_Close(s_Data.m_HostContext);
			s_Data.m_HostContext = nullptr;
		}

		// Reload
		return LoadAssembly(s_Data.m_AssemblyPath);
	}

	void* ScriptEngine::GetFunction(const std::string& typeName, const std::string& methodName)
	{
		if (!s_Data.m_LoadAssemblyAndGetFunctionPointer)
		{
			GX_CORE_ERROR("[ScriptEngine] Assembly loader delegate not initialized");
			return nullptr;
		}

		// Ensure we have an absolute path
		std::filesystem::path absoluteAssemblyPath = std::filesystem::absolute(s_Data.m_AssemblyPath);

		// Convert strings to wide strings on Windows
#ifdef ENGINE_PLATFORM_WINDOWS
		std::wstring wAssemblyPath(absoluteAssemblyPath.wstring());
		std::wstring wTypeName(typeName.begin(), typeName.end());
		std::wstring wMethodName(methodName.begin(), methodName.end());
#else
		std::string wAssemblyPath(absoluteAssemblyPath.string());
		std::string wTypeName(typeName);
		std::string wMethodName(methodName);
#endif

		GX_CORE_INFO("[ScriptEngine] Attempting to get function: {}::{} from {}",
			typeName, methodName, absoluteAssemblyPath.string());

		// Get the function pointer
		void* functionPtr = nullptr;
		int32_t result = s_Data.m_LoadAssemblyAndGetFunctionPointer(
			wAssemblyPath.c_str(),
			wTypeName.c_str(),
			wMethodName.c_str(),
			UNMANAGEDCALLERSONLY_METHOD, // Special value for UnmanagedCallersOnlyAttribute methods
			nullptr, // Reserved
			&functionPtr
		);

		if (result != 0 || functionPtr == nullptr)
		{
			GX_CORE_ERROR("[ScriptEngine] Failed to get function pointer: {}::{} (error code: {})",
				typeName, methodName, result);
			return nullptr;
		}

		GX_CORE_INFO("[ScriptEngine] Successfully retrieved function pointer: {}::{}", typeName, methodName);
		return functionPtr;
	}

	// --- Private Implementation ---
	void ScriptEngine::InitDotNet(const std::filesystem::path& assemblyPath)
	{
		GX_CORE_INFO("[ScriptEngine] Initializing .NET Runtime");
		s_Data = ScriptEngineData{};

		LoadAssembly(assemblyPath);
	}

	void ScriptEngine::ShutdownDotNet()
	{
		if (s_Data.m_HostContext && s_Data.m_Close)
		{
			s_Data.m_Close(s_Data.m_HostContext);
			s_Data.m_HostContext = nullptr;
		}

#ifdef ENGINE_PLATFORM_WINDOWS
		if (s_Data.m_HostfxrLib)
		{
			::FreeLibrary((HMODULE)s_Data.m_HostfxrLib);
			s_Data.m_HostfxrLib = nullptr;
		}
#else
		if (s_Data.m_HostfxrLib)
		{
			dlclose(s_Data.m_HostfxrLib);
			s_Data.m_HostfxrLib = nullptr;
		}
#endif

		std::cout << "[ScriptEngine] .NET Runtime shutdown complete\n";
	}

	bool ScriptEngine::LoadHostFxr()
	{
		// For self-contained: Look for hostfxr next to the assembly
		// For framework-dependent: Use get_hostfxr_path

		// Try self-contained first (hostfxr.dll/so in app directory)
		std::filesystem::path appDir = std::filesystem::current_path();

#ifdef ENGINE_PLATFORM_WINDOWS
		std::filesystem::path hostfxrPath = appDir / "hostfxr.dll";
#elif defined(__APPLE__)
		std::filesystem::path hostfxrPath = appDir / "libhostfxr.dylib";
#else
		std::filesystem::path hostfxrPath = appDir / "libhostfxr.so";
#endif

		bool isGlobalHostfxr = false;

		// If not found locally, try specific .NET 9.0 version first, then fall back
		if (!std::filesystem::exists(hostfxrPath))
		{
#ifdef ENGINE_PLATFORM_WINDOWS
			// Try .NET 9.0 specific path first
			std::vector<std::wstring> dotnet9Paths = {
				L"C:\\Program Files\\dotnet\\host\\fxr\\9.0.11\\hostfxr.dll",
				L"C:\\Program Files\\dotnet\\host\\fxr\\9.0.10\\hostfxr.dll",
				L"C:\\Program Files\\dotnet\\host\\fxr\\9.0.0\\hostfxr.dll"
			};

			bool found = false;
			for (const auto& path : dotnet9Paths)
			{
				if (std::filesystem::exists(path))
				{
					hostfxrPath = path;
					found = true;
					GX_CORE_INFO("[ScriptEngine] Using .NET 9.0 hostfxr: {}", hostfxrPath.string());
					break;
				}
			}

			// If no .NET 9 found, fall back to system default
			if (!found)
#endif
			{
				char_t buffer[MAX_PATH];
				size_t bufferSize = sizeof(buffer) / sizeof(char_t);

				int32_t result = get_hostfxr_path(buffer, &bufferSize, nullptr);
				if (result != 0)
				{
					GX_CORE_ERROR("[ScriptEngine] get_hostfxr_path failed (error code: {0})", result);
					return false;
				}

				hostfxrPath = buffer;
				GX_CORE_INFO("[ScriptEngine] Using default hostfxr: {}", hostfxrPath.string());
			}
			isGlobalHostfxr = true;
		}
		else
		{
			GX_CORE_INFO("[ScriptEngine] Using local hostfxr: {}", hostfxrPath.string());
		}

		// Load the hostfxr library
#ifdef ENGINE_PLATFORM_WINDOWS
		s_Data.m_HostfxrLib = LoadNativeLibrary(hostfxrPath.wstring().c_str());
#else
		s_Data.m_HostfxrLib = LoadNativeLibrary(hostfxrPath.c_str());
#endif

		if (!s_Data.m_HostfxrLib)
		{
			GX_CORE_ERROR("[ScriptEngine] Failed to load library");
			return false;
		}

		// Get function pointers - USE RUNTIME CONFIG VERSION for framework-dependent
		s_Data.m_InitFxrForRuntimeConfig = (HostfxrInitializeForRuntimeConfigFn)
			GetExport(s_Data.m_HostfxrLib, "hostfxr_initialize_for_runtime_config");

		s_Data.m_GetRuntimeDelegate = (HostfxrGetRuntimeDelegateFn)
			GetExport(s_Data.m_HostfxrLib, "hostfxr_get_runtime_delegate");

		s_Data.m_Close = (HostfxrCloseFn)
			GetExport(s_Data.m_HostfxrLib, "hostfxr_close");

		s_Data.m_SetErrorWriter = (HostfxrSetErrorWriterFn)
			GetExport(s_Data.m_HostfxrLib, "hostfxr_set_error_writer");

		s_Data.m_SetRuntimePropertyValue = (HostfxrSetRuntimePropertyValueFn)
			GetExport(s_Data.m_HostfxrLib, "hostfxr_set_runtime_property_value");

		if (!s_Data.m_InitFxrForRuntimeConfig || !s_Data.m_GetRuntimeDelegate || !s_Data.m_Close)
		{
			GX_CORE_ERROR("[ScriptEngine] Failed to load required hostfxr functions");
			return false;
		}

		// Set error writer
		if (s_Data.m_SetErrorWriter)
		{
			s_Data.m_SetErrorWriter(HostfxrErrorWriter);
		}

		GX_CORE_INFO("[ScriptEngine] Successfully loaded hostfxr");		return true;
	}

	bool ScriptEngine::InitializeHostFxrContext(const std::filesystem::path& assemblyPath)
	{
		// For framework-dependent deployment, use runtime config initialization
		// Find the .runtimeconfig.json file (same name as DLL but with .runtimeconfig.json extension)
		std::filesystem::path runtimeConfigPath = assemblyPath;
		runtimeConfigPath.replace_extension(".runtimeconfig.json");

		if (!std::filesystem::exists(runtimeConfigPath))
		{
			GX_CORE_ERROR("[ScriptEngine] Runtime config not found: {}", runtimeConfigPath.string());
			return false;
		}

		GX_CORE_INFO("[ScriptEngine] Using runtime config: {}", runtimeConfigPath.string());

#ifdef ENGINE_PLATFORM_WINDOWS
		std::wstring wRuntimeConfigPath = runtimeConfigPath.wstring();
#else
		std::string wRuntimeConfigPath = runtimeConfigPath.string();
#endif

		int32_t result = s_Data.m_InitFxrForRuntimeConfig(
			wRuntimeConfigPath.c_str(),
			nullptr, // No additional parameters needed
			&s_Data.m_HostContext
		);

		if (result != 0 || s_Data.m_HostContext == nullptr)
		{
			GX_CORE_ERROR("[ScriptEngine] hostfxr_initialize_for_runtime_config failed (error code: {0})", result);
			return false;
		}

		GX_CORE_INFO("[ScriptEngine] Successfully initialized hostfxr context");
		return true;
	}

	bool ScriptEngine::LoadAssemblyAndGetFunctionPointer()
	{
		void* loadAssemblyDelegate = nullptr;
		int32_t result = s_Data.m_GetRuntimeDelegate(
			s_Data.m_HostContext,
			hdt_load_assembly_and_get_function_pointer,
			&loadAssemblyDelegate
		);

		if (result != 0 || loadAssemblyDelegate == nullptr)
		{
			GX_CORE_ERROR("[ScriptEngine] Failed to get load_assembly_and_get_function_pointer delegate (error code: {0})", result);
			return false;
		}

		s_Data.m_LoadAssemblyAndGetFunctionPointer =
			(load_assembly_and_get_function_pointer_fn)loadAssemblyDelegate;

		// Trigger module initialization by loading the assembly
		// This will cause the ModuleInitializer to run
		std::filesystem::path absoluteAssemblyPath = std::filesystem::absolute(s_Data.m_AssemblyPath);

#ifdef ENGINE_PLATFORM_WINDOWS
		std::wstring wAssemblyPath(absoluteAssemblyPath.wstring());
		std::wstring wTypeName(L"GravixEngine.ModuleInitializer, GravixScripting");
		std::wstring wMethodName(L"Initialize");
#else
		std::string wAssemblyPath(absoluteAssemblyPath.string());
		std::string wTypeName("GravixEngine.ModuleInitializer, GravixScripting");
		std::string wMethodName("Initialize");
#endif

		GX_CORE_INFO("[ScriptEngine] Triggering module initialization");

		// Try to get the Initialize function to trigger assembly load and module initializer
		void* initFunctionPtr = nullptr;
		int32_t initResult = s_Data.m_LoadAssemblyAndGetFunctionPointer(
			wAssemblyPath.c_str(),
			wTypeName.c_str(),
			wMethodName.c_str(),
			UNMANAGEDCALLERSONLY_METHOD,
			nullptr,
			&initFunctionPtr
		);

		// Note: This might fail if Initialize is not marked with UnmanagedCallersOnly,
		// but the ModuleInitializer should still run during assembly load
		if (initResult == 0 && initFunctionPtr != nullptr)
		{
			GX_CORE_INFO("[ScriptEngine] Module initializer function found");
		}
		else
		{
			GX_CORE_INFO("[ScriptEngine] Module initializer will run automatically on first function call");
		}

		return true;
	}

	ScriptInstance ScriptEngine::CreateInstance(const std::string& typeName)
	{
		typedef void* (*CreateScriptFn)(const char*);
		static auto createScript = (CreateScriptFn)GetFunction(
			"GravixEngine.Interop.ScriptInstanceManager, GravixScripting",
			"CreateScript"
		);

		if (!createScript)
		{
			GX_CORE_ERROR("[ScriptEngine] Failed to get CreateScript function");
			return ScriptInstance();
		}

		std::string fullTypeName = typeName + ", GravixScripting";
		void* handle = createScript(fullTypeName.c_str());

		if (!handle)
		{
			GX_CORE_ERROR("[ScriptEngine] Failed to create instance of type: {}", typeName);
			return ScriptInstance();
		}

		GX_CORE_INFO("[ScriptEngine] Successfully created instance of type: {}", typeName);
		return ScriptInstance(handle, typeName);
	}
}