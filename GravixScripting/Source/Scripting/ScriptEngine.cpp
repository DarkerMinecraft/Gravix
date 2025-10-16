#include "ScriptEngine.h"

#ifdef ENGINE_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include <nethost.h>
#include <coreclr_delegates.h>
#include <hostfxr.h>
#include <iostream>
#include <minwindef.h>

#ifdef ENGINE_PLATFORM_WINDOWS
#define STR(s) L ## s
#define CH(c) L ## c
#define DIR_SEPARATOR L'\\'
#else
#define STR(s) s
#define CH(c) c
#define DIR_SEPARATOR '/'
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

	// --- ScriptEngine Data ---
	struct ScriptEngineData
	{
		void* m_HostfxrLib = nullptr;
		hostfxr_handle m_HostContext = nullptr;

		HostfxrInitializeForRuntimeConfigFn m_InitFxrForConfig = nullptr;
		HostfxrGetRuntimeDelegateFn m_GetRuntimeDelegate = nullptr;
		HostfxrCloseFn m_Close = nullptr;
		HostfxrSetErrorWriterFn m_SetErrorWriter = nullptr;

		load_assembly_and_get_function_pointer_fn m_LoadAssemblyAndGetFunctionPointer = nullptr;

		std::filesystem::path m_AssemblyPath;
		std::filesystem::path m_RuntimeConfigPath;
	};

	static ScriptEngineData s_Data;

	// --- Helper: Load Native Library ---
	static void* LoadNativeLibrary(const char_t* path)
	{
#ifdef ENGINE_PLATFORM_WINDOWS
		HMODULE h = ::LoadLibraryW(path);
		if (!h)
			std::cerr << "[ScriptEngine] Failed to load library\n";
		return (void*)h;
#else
		void* h = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
		if (!h)
			std::cerr << "[ScriptEngine] Failed to load library: " << dlerror() << "\n";
		return h;
#endif
	}

	// --- Helper: Get Export from Library ---
	static void* GetExport(void* lib, const char* name)
	{
#ifdef ENGINE_PLATFORM_WINDOWS
		void* f = ::GetProcAddress((HMODULE)lib, name);
		if (!f)
			std::cerr << "[ScriptEngine] Failed to get export: " << name << "\n";
		return f;
#else
		void* f = dlsym(lib, name);
		if (!f)
			std::cerr << "[ScriptEngine] Failed to get export: " << dlerror() << "\n";
		return f;
#endif
	}

	// --- Error Writer Callback ---
	static void HostfxrErrorWriter(const char_t* message)
	{
#ifdef ENGINE_PLATFORM_WINDOWS
		std::wstring wstr(message);
		std::string str(wstr.begin(), wstr.end());
		std::cerr << "[HOSTFXR] " << str << "\n";
#else
		std::cerr << "[HOSTFXR] " << message << "\n";
#endif
	}

	// --- Public API ---
	void ScriptEngine::Init()
	{
		std::cout << "[ScriptEngine] Initializing Script Engine\n";
		InitDotNet();
	}

	void ScriptEngine::Shutdown()
	{
		std::cout << "[ScriptEngine] Shutting down Script Engine\n";
		ShutdownDotNet();
	}

	bool ScriptEngine::LoadAssembly(const std::filesystem::path& assemblyPath)
	{
		if (!std::filesystem::exists(assemblyPath))
		{
			std::cerr << "[ScriptEngine] Assembly not found: " << assemblyPath.string() << "\n";
			return false;
		}

		s_Data.m_AssemblyPath = assemblyPath;

		// Look for .runtimeconfig.json next to the assembly
		std::filesystem::path runtimeConfig = assemblyPath;
		runtimeConfig.replace_extension(".runtimeconfig.json");

		if (!std::filesystem::exists(runtimeConfig))
		{
			std::cout << "[ScriptEngine] Runtime config not found: " << runtimeConfig.string() << "\n";
			std::cout << "[ScriptEngine] Assembly may still load if it's self-contained\n";
		}
		else
		{
			s_Data.m_RuntimeConfigPath = runtimeConfig;
		}

		// Load hostfxr if not already loaded
		if (!s_Data.m_HostfxrLib && !LoadHostFxr())
		{
			std::cerr << "[ScriptEngine] Failed to load hostfxr\n";
			return false;
		}

		// Initialize the host context for this assembly
		if (!InitializeHostFxrContext(assemblyPath))
		{
			std::cerr << "[ScriptEngine] Failed to initialize hostfxr context\n";
			return false;
		}

		// Get the load_assembly_and_get_function_pointer delegate
		if (!LoadAssemblyAndGetFunctionPointer())
		{
			std::cerr << "[ScriptEngine] Failed to get assembly loader delegate\n";
			return false;
		}

		std::cout << "[ScriptEngine] Successfully loaded assembly: " << assemblyPath.filename().string() << "\n";
		return true;
	}

	bool ScriptEngine::ReloadAssembly()
	{
		if (s_Data.m_AssemblyPath.empty())
		{
			std::cerr << "[ScriptEngine] No assembly loaded to reload\n";
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
			std::cerr << "[ScriptEngine] Assembly loader not initialized\n";
			return nullptr;
		}

		// Convert strings to wide strings on Windows
#ifdef ENGINE_PLATFORM_WINDOWS
		std::wstring wAssemblyPath(s_Data.m_AssemblyPath.wstring());
		std::wstring wTypeName(typeName.begin(), typeName.end());
		std::wstring wMethodName(methodName.begin(), methodName.end());
#else
		std::string wAssemblyPath(s_Data.m_AssemblyPath.string());
		std::string wTypeName(typeName);
		std::string wMethodName(methodName);
#endif

		// Get the function pointer
		void* functionPtr = nullptr;
		int32_t result = s_Data.m_LoadAssemblyAndGetFunctionPointer(
			wAssemblyPath.c_str(),
			wTypeName.c_str(),
			wMethodName.c_str(),
			nullptr, // Delegate type name (nullptr for UnmanagedCallersOnlyAttribute)
			nullptr, // Reserved
			&functionPtr
		);

		if (result != 0 || functionPtr == nullptr)
		{
			std::cerr << "[ScriptEngine] Failed to get function pointer: " << typeName
				<< "::" << methodName << " (error code: " << result << ")\n";
			return nullptr;
		}

		std::cout << "[ScriptEngine] Successfully loaded function: " << typeName << "::" << methodName << "\n";
		return functionPtr;
	}

	// --- Private Implementation ---
	void ScriptEngine::InitDotNet()
	{
		std::cout << "[ScriptEngine] Initializing .NET Runtime\n";

		// We'll load assemblies on-demand, so just prepare the system
		s_Data = ScriptEngineData{};
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
		// Get the path to hostfxr
		char_t buffer[MAX_PATH];
		size_t bufferSize = sizeof(buffer) / sizeof(char_t);

		int32_t result = get_hostfxr_path(buffer, &bufferSize, nullptr);
		if (result != 0)
		{
			std::cerr << "[ScriptEngine] Failed to get hostfxr path (error code: " << result << ")\n";
			return false;
		}

		// Load the hostfxr library
		s_Data.m_HostfxrLib = LoadNativeLibrary(buffer);
		if (!s_Data.m_HostfxrLib)
		{
			std::cerr << "[ScriptEngine] Failed to load hostfxr library\n";
			return false;
		}

		// Get function pointers
		s_Data.m_InitFxrForConfig = (HostfxrInitializeForRuntimeConfigFn)
			GetExport(s_Data.m_HostfxrLib, "hostfxr_initialize_for_runtime_config");

		s_Data.m_GetRuntimeDelegate = (HostfxrGetRuntimeDelegateFn)
			GetExport(s_Data.m_HostfxrLib, "hostfxr_get_runtime_delegate");

		s_Data.m_Close = (HostfxrCloseFn)
			GetExport(s_Data.m_HostfxrLib, "hostfxr_close");

		s_Data.m_SetErrorWriter = (HostfxrSetErrorWriterFn)
			GetExport(s_Data.m_HostfxrLib, "hostfxr_set_error_writer");

		if (!s_Data.m_InitFxrForConfig || !s_Data.m_GetRuntimeDelegate || !s_Data.m_Close)
		{
			std::cerr << "[ScriptEngine] Failed to load required hostfxr functions\n";
			return false;
		}

		// Set error writer
		if (s_Data.m_SetErrorWriter)
		{
			s_Data.m_SetErrorWriter(HostfxrErrorWriter);
		}

		std::cout << "[ScriptEngine] Successfully loaded hostfxr\n";
		return true;
	}

	bool ScriptEngine::InitializeHostFxrContext(const std::filesystem::path& assemblyPath)
	{
		// Use runtime config if available, otherwise use assembly path
		std::filesystem::path configPath = s_Data.m_RuntimeConfigPath.empty()
			? assemblyPath
			: s_Data.m_RuntimeConfigPath;

#ifdef ENGINE_PLATFORM_WINDOWS
		std::wstring wConfigPath = configPath.wstring();
#else
		std::string wConfigPath = configPath.string();
#endif

		int32_t result = s_Data.m_InitFxrForConfig(
			wConfigPath.c_str(),
			nullptr,
			&s_Data.m_HostContext
		);

		if (result != 0 || s_Data.m_HostContext == nullptr)
		{
			std::cerr << "[ScriptEngine] Failed to initialize runtime config (error code: " << result << ")\n";
			return false;
		}

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
			std::cerr << "[ScriptEngine] Failed to get load_assembly_and_get_function_pointer delegate (error code: "
				<< result << ")\n";
			return false;
		}

		s_Data.m_LoadAssemblyAndGetFunctionPointer =
			(load_assembly_and_get_function_pointer_fn)loadAssemblyDelegate;

		return true;
	}
}