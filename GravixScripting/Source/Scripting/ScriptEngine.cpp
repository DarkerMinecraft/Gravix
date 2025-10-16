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
	using HostfxrInitializeForDotnetCommandLineFn = int32_t(*)(
		int argc,
		const char_t** argv,
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

		HostfxrInitializeForDotnetCommandLineFn m_InitFxrForCommandLine = nullptr;
		HostfxrGetRuntimeDelegateFn m_GetRuntimeDelegate = nullptr;
		HostfxrCloseFn m_Close = nullptr;
		HostfxrSetErrorWriterFn m_SetErrorWriter = nullptr;

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
		InitDotNet("GravixScripting.dll");
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

		// Load hostfxr if not already loaded
		if (!s_Data.m_HostfxrLib && !LoadHostFxr())
		{
			std::cerr << "[ScriptEngine] Failed to load hostfxr\n";
			return false;
		}

		// Initialize the host context for this assembly (self-contained mode)
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
	void ScriptEngine::InitDotNet(const std::filesystem::path& assemblyPath)
	{
		std::cout << "[ScriptEngine] Initializing .NET Runtime\n";
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

		// If not found locally, try to get system hostfxr
		if (!std::filesystem::exists(hostfxrPath))
		{
			char_t buffer[MAX_PATH];
			size_t bufferSize = sizeof(buffer) / sizeof(char_t);

			int32_t result = get_hostfxr_path(buffer, &bufferSize, nullptr);
			if (result != 0)
			{
				std::cerr << "[ScriptEngine] Failed to get hostfxr path (error code: " << result << ")\n";
				std::cerr << "[ScriptEngine] Make sure to publish your .NET assembly as self-contained\n";
				return false;
			}

			hostfxrPath = buffer;
			isGlobalHostfxr = true;
			std::cout << "[ScriptEngine] Using system hostfxr: " << hostfxrPath << "\n";
		}
		else
		{
			std::cout << "[ScriptEngine] Using local hostfxr (self-contained): " << hostfxrPath << "\n";
		}

		// Load the hostfxr library
#ifdef ENGINE_PLATFORM_WINDOWS
		s_Data.m_HostfxrLib = LoadNativeLibrary(hostfxrPath.wstring().c_str());
#else
		s_Data.m_HostfxrLib = LoadNativeLibrary(hostfxrPath.c_str());
#endif

		if (!s_Data.m_HostfxrLib)
		{
			std::cerr << "[ScriptEngine] Failed to load hostfxr library from: " << hostfxrPath << "\n";
			return false;
		}

		// Get function pointers - USE COMMAND LINE VERSION for self-contained
		s_Data.m_InitFxrForCommandLine = (HostfxrInitializeForDotnetCommandLineFn)
			GetExport(s_Data.m_HostfxrLib, "hostfxr_initialize_for_dotnet_command_line");

		s_Data.m_GetRuntimeDelegate = (HostfxrGetRuntimeDelegateFn)
			GetExport(s_Data.m_HostfxrLib, "hostfxr_get_runtime_delegate");

		s_Data.m_Close = (HostfxrCloseFn)
			GetExport(s_Data.m_HostfxrLib, "hostfxr_close");

		s_Data.m_SetErrorWriter = (HostfxrSetErrorWriterFn)
			GetExport(s_Data.m_HostfxrLib, "hostfxr_set_error_writer");

		if (!s_Data.m_InitFxrForCommandLine || !s_Data.m_GetRuntimeDelegate || !s_Data.m_Close)
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
		// For self-contained deployment, use command line initialization
		std::filesystem::path appDir = assemblyPath.parent_path();

#ifdef ENGINE_PLATFORM_WINDOWS
		std::wstring wAssemblyPath = assemblyPath.wstring();
		std::wstring wAppDir = appDir.wstring();
		const char_t* argv[] = { wAssemblyPath.c_str() };
#else
		std::string sAssemblyPath = assemblyPath.string();
		std::string sAppDir = appDir.string();
		const char_t* argv[] = { sAssemblyPath.c_str() };
#endif

		// Setup initialization parameters for self-contained
		hostfxr_initialize_parameters params;
		params.size = sizeof(hostfxr_initialize_parameters);
		params.host_path = argv[0];
#ifdef ENGINE_PLATFORM_WINDOWS
		params.dotnet_root = wAppDir.c_str();
#else
		params.dotnet_root = sAppDir.c_str();
#endif

		int32_t result = s_Data.m_InitFxrForCommandLine(
			1,
			argv,
			&params,
			&s_Data.m_HostContext
		);

		if (result != 0 || s_Data.m_HostContext == nullptr)
		{
			std::cerr << "[ScriptEngine] Failed to initialize hostfxr context (error code: " << result << ")\n";
			std::cerr << "[ScriptEngine] Make sure assembly is published as self-contained with .NET 9\n";
			return false;
		}

		std::cout << "[ScriptEngine] Successfully initialized hostfxr context (self-contained mode)\n";
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