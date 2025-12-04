#pragma once

#include <filesystem>

namespace Gravix 
{

	class FileDialogs
	{
	public:
		static std::filesystem::path OpenFile(const char* filter);
		static std::filesystem::path SaveFile(const char* filter);
		static std::filesystem::path OpenFolder(const char* title = "Select Folder");

#ifdef ENGINE_PLATFORM_WINDOWS
		// Overload for custom owner window (for splash screens, etc.)
		static std::filesystem::path OpenFolderWithOwner(void* ownerHwnd, const char* title = "Select Folder");
#endif
	};

}