#include "pch.h"
#include "Project.h"

#ifdef GRAVIX_EDITOR_BUILD
#include "Serialization/Project/ProjectSerializer.h"
#endif
#include "Asset/EditorAssetManager.h"
#include "Core/Console.h"

#ifdef ENGINE_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include <sstream>
#include <fstream>

namespace Gravix 
{

	Ref<Project> Project::New()
	{
		s_ActiveProject = CreateRef<Project>();

		// Initialize with default values but no working directory
		s_ActiveProject->m_Config.StartScene = 0; // Null handle

		// Initialize asset manager
		Ref<EditorAssetManager> editorAssetManager = CreateRef<EditorAssetManager>();
		s_ActiveProject->m_AssetManager = editorAssetManager;

		return s_ActiveProject;
	}

	Ref<Project> Project::New(const std::filesystem::path& workingDirectory)
	{
		s_ActiveProject = CreateRef<Project>();

		// Set working directory
		s_ActiveProject->m_WorkingDirectory = workingDirectory;

		// Set default configuration values
		s_ActiveProject->m_Config.Name = "Untitled";
		s_ActiveProject->m_Config.StartScene = 0; // Null handle
		s_ActiveProject->m_Config.AssetDirectory = workingDirectory / "Assets";
		s_ActiveProject->m_Config.LibraryDirectory = workingDirectory / "Library";
		s_ActiveProject->m_Config.ScriptPath = workingDirectory / "Scripts";

		// Create directories if they don't exist
		if (!std::filesystem::exists(s_ActiveProject->m_Config.AssetDirectory))
		{
			std::filesystem::create_directories(s_ActiveProject->m_Config.AssetDirectory);
			GX_CORE_INFO("Created Assets directory: {}", s_ActiveProject->m_Config.AssetDirectory.string());
		}
		else
		{
			GX_CORE_INFO("Assets directory already exists: {}", s_ActiveProject->m_Config.AssetDirectory.string());
		}

		if (!std::filesystem::exists(s_ActiveProject->m_Config.LibraryDirectory))
		{
			std::filesystem::create_directories(s_ActiveProject->m_Config.LibraryDirectory);
			GX_CORE_INFO("Created Library directory: {}", s_ActiveProject->m_Config.LibraryDirectory.string());
		}
		else
		{
			GX_CORE_INFO("Library directory already exists: {}", s_ActiveProject->m_Config.LibraryDirectory.string());
		}

		if (!std::filesystem::exists(s_ActiveProject->m_Config.ScriptPath))
		{
			std::filesystem::create_directories(s_ActiveProject->m_Config.ScriptPath);
			GX_CORE_INFO("Created Scripts directory: {}", s_ActiveProject->m_Config.ScriptPath.string());
		}
		else
		{
			GX_CORE_INFO("Scripts directory already exists: {}", s_ActiveProject->m_Config.ScriptPath.string());
		}

		// Initialize asset manager
		Ref<EditorAssetManager> editorAssetManager = CreateRef<EditorAssetManager>();
		s_ActiveProject->m_AssetManager = editorAssetManager;

		// Setup scripting environment (create Sandbox.csproj and copy GravixScripting.dll)
		s_ActiveProject->SetupScriptingEnvironment();

		return s_ActiveProject;
	}

#ifdef GRAVIX_EDITOR_BUILD
	Ref<Project> Project::Load(const std::filesystem::path& path)
	{
		Ref<Project> project = CreateRef<Project>();
		ProjectSerializer serializer(project);
		if (serializer.Deserialize(path))
		{
			// Convert relative paths to absolute paths
			project->GetConfig().AssetDirectory = path.parent_path() / project->GetConfig().AssetDirectory;
			project->GetConfig().LibraryDirectory = path.parent_path() / project->GetConfig().LibraryDirectory;
			project->GetConfig().ScriptPath = path.parent_path() / project->GetConfig().ScriptPath;

			// Create directories if they don't exist
			if (!std::filesystem::exists(project->GetConfig().AssetDirectory))
				std::filesystem::create_directories(project->GetConfig().AssetDirectory);
			if (!std::filesystem::exists(project->GetConfig().LibraryDirectory))
				std::filesystem::create_directories(project->GetConfig().LibraryDirectory);
			if (!std::filesystem::exists(project->GetConfig().ScriptPath))
				std::filesystem::create_directories(project->GetConfig().ScriptPath);

			s_ActiveProject = project;
			Ref<EditorAssetManager> editorAssetManager = CreateRef<EditorAssetManager>();
			s_ActiveProject->m_AssetManager = editorAssetManager;
			editorAssetManager->DeserializeAssetRegistry();
			s_ActiveProject->m_WorkingDirectory = path.parent_path();

			// Setup scripting environment (create Sandbox.csproj and copy GravixScripting.dll)
			s_ActiveProject->SetupScriptingEnvironment();

			return s_ActiveProject;
		}

		return nullptr;
	}

	void Project::SaveActive(const std::filesystem::path& path)
	{
		ProjectSerializer serializer(s_ActiveProject);
		serializer.Serialize(path);
	}
#endif

	void Project::SetupScriptingEnvironment()
	{
		// Create Scripts/bin directory if it doesn't exist
		std::filesystem::path scriptBinPath = m_Config.ScriptPath / "bin";
		if (!std::filesystem::exists(scriptBinPath))
		{
			std::filesystem::create_directories(scriptBinPath);
			GX_CORE_INFO("Created Scripts/bin directory: {}", scriptBinPath.string());
		}

		// Handle renaming: remove old .csproj files with different names
		for (const auto& entry : std::filesystem::directory_iterator(m_Config.ScriptPath))
		{
			if (entry.path().extension() == ".csproj")
			{
				std::string existingName = entry.path().stem().string();
				if (existingName != m_Config.Name)
				{
					std::filesystem::remove(entry.path());
					GX_CORE_INFO("Removed old .csproj file: {}", entry.path().string());
				}
			}
		}

		// Create [ProjectName].csproj if it doesn't exist
		std::filesystem::path csprojPath = m_Config.ScriptPath / (m_Config.Name + ".csproj");
		if (!std::filesystem::exists(csprojPath))
		{
			std::ofstream csprojFile(csprojPath);
			if (csprojFile.is_open())
			{
				csprojFile << R"xml(<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <!-- Build a DLL -->
    <OutputType>Library</OutputType>
    <TargetFramework>net48</TargetFramework>

    <!-- Enable recursive include -->
    <EnableDefaultItems>false</EnableDefaultItems>

    <!-- Mono compatibility -->
    <UseMscorlib>true</UseMscorlib>
    <DisableImplicitFrameworkReferences>true</DisableImplicitFrameworkReferences>

    <!-- Allow older C# syntax required by mcs -->
    <LangVersion>7.3</LangVersion>

    <!-- Output paths -->
    <AssemblyName>)xml" << m_Config.Name << R"xml(</AssemblyName>
    <OutputPath>bin/</OutputPath>
    <AppendTargetFrameworkToOutputPath>false</AppendTargetFrameworkToOutputPath>
  </PropertyGroup>

  <!-- Recursive source include from Assets directory -->
  <ItemGroup>
    <Compile Include="../Assets/**/*.cs" />
  </ItemGroup>

  <!-- Reference to GravixScripting.dll (engine core) -->
  <ItemGroup>
    <Reference Include="GravixScripting">
      <HintPath>bin\GravixScripting.dll</HintPath>
      <Private>true</Private>
    </Reference>
  </ItemGroup>

  <!-- Mono system libraries -->
  <ItemGroup>
    <Reference Include="System" />
    <Reference Include="System.Core" />
    <Reference Include="System.Xml" />
    <Reference Include="System.Runtime" />
  </ItemGroup>

</Project>
)xml";
				csprojFile.close();
				GX_CORE_INFO("Created {}.csproj: {}", m_Config.Name, csprojPath.string());
			}
			else
			{
				GX_CORE_ERROR("Failed to create {}.csproj at: {}", m_Config.Name, csprojPath.string());
			}
		}

		// Always copy GravixScripting.dll to Scripts/bin
		// Get the current executable directory
		std::filesystem::path exePath = std::filesystem::current_path();
		std::filesystem::path sourceDllPath = exePath / "GravixScripting.dll";
		std::filesystem::path destDllPath = scriptBinPath / "GravixScripting.dll";

		if (std::filesystem::exists(sourceDllPath))
		{
			try
			{
				// Copy the DLL, always overwriting
				std::filesystem::copy_file(sourceDllPath, destDllPath, std::filesystem::copy_options::overwrite_existing);
				GX_CORE_INFO("Copied GravixScripting.dll to: {}", destDllPath.string());
			}
			catch (const std::filesystem::filesystem_error& e)
			{
				GX_CORE_ERROR("Failed to copy GravixScripting.dll: {}", e.what());
			}
		}
		else
		{
			GX_CORE_ERROR("GravixScripting.dll not found at: {} - Cannot setup scripting environment!", sourceDllPath.string());
		}

		// Build the game scripts DLL using dotnet build silently
		GX_CORE_INFO("Building game scripts...");

		std::string buildCommand = "dotnet build \"" + csprojPath.string() + "\" -c Release --nologo";
		std::string buildOutput;
		int buildResult = 0;
		bool hasErrors = false;
		int errorCount = 0;

#ifdef ENGINE_PLATFORM_WINDOWS
		// Create temporary file for output
		char tempPath[MAX_PATH];
		GetTempPathA(MAX_PATH, tempPath);
		std::string outputFile = std::string(tempPath) + "gravix_build_output.txt";

		// Use PowerShell to run dotnet build silently (no console window)
		std::string psCommand = "powershell.exe -WindowStyle Hidden -NoProfile -Command \"" +
								buildCommand + " 2>&1 | Out-File -FilePath '" + outputFile + "' -Encoding UTF8\"";

		STARTUPINFOA si = {};
		si.cb = sizeof(STARTUPINFOA);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;

		PROCESS_INFORMATION pi = {};

		// CREATE_NO_WINDOW prevents console window
		if (!CreateProcessA(NULL, (LPSTR)psCommand.c_str(), NULL, NULL, FALSE,
			CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
		{
			Console::LogError("Failed to run dotnet build. Is .NET SDK installed?");
			GX_CORE_ERROR("Failed to run dotnet build");
			return;
		}

		// Wait for process to finish
		WaitForSingleObject(pi.hProcess, INFINITE);

		DWORD exitCode;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		buildResult = (int)exitCode;

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		// Read output from temp file
		std::ifstream outFile(outputFile);
		if (outFile.is_open())
		{
			buildOutput.assign((std::istreambuf_iterator<char>(outFile)),
							   std::istreambuf_iterator<char>());
			outFile.close();
			DeleteFileA(outputFile.c_str());
		}
#else
		// Fallback for non-Windows platforms
		FILE* pipe = popen(buildCommand.c_str(), "r");
		if (!pipe)
		{
			Console::LogError("Failed to run dotnet build. Is .NET SDK installed?");
			GX_CORE_ERROR("Failed to run dotnet build");
			return;
		}

		char buffer[256];
		while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
		{
			buildOutput += buffer;
		}

		buildResult = pclose(pipe);
#endif

		// Parse build output for errors and warnings
		std::istringstream outputStream(buildOutput);
		std::string line;
		while (std::getline(outputStream, line))
		{
			// Check for actual C# compilation errors
			if (line.find(": error CS") != std::string::npos)
			{
				hasErrors = true;
				errorCount++;
				Console::LogError(line);
				GX_CORE_ERROR("{}", line);
			}
			// Check for warnings
			else if (line.find(": warning CS") != std::string::npos)
			{
				Console::LogWarning(line);
				GX_CORE_WARN("{}", line);
			}
		}

		if (buildResult == 0 && !hasErrors)
		{
			std::filesystem::path outputDll = scriptBinPath / (m_Config.Name + ".dll");
			if (std::filesystem::exists(outputDll))
			{
				GX_CORE_INFO("Successfully built {}.dll (game scripts)", m_Config.Name);
			}
			else
			{
				GX_CORE_WARN("Build completed but {}.dll not found at expected location: {}", m_Config.Name, outputDll.string());
			}
		}
		else
		{
			std::string errorMsg = "Failed to build game scripts with " + std::to_string(errorCount) + " error(s)";
			Console::LogError(errorMsg);
			GX_CORE_ERROR("{}", errorMsg);
		}
	}

}