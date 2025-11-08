#pragma once

#include "Utils/PlatformUtils.h"
#include "Core/Application.h"

#include <commdlg.h>
#include <shlobj.h>

namespace Gravix 
{

	std::filesystem::path FileDialogs::OpenFile(const char* filter) 
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };

		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = (HWND)Application::Get().GetWindow().GetWindowHandle();
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetOpenFileNameA(&ofn) == TRUE) 
		{
			return std::filesystem::path(ofn.lpstrFile);
		}
		return std::filesystem::path();
	}

	std::filesystem::path FileDialogs::SaveFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };

		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = (HWND)Application::Get().GetWindow().GetWindowHandle();
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetSaveFileNameA(&ofn) == TRUE)
		{
			return std::filesystem::path(ofn.lpstrFile);
		}
		return std::filesystem::path();
	}

	std::filesystem::path FileDialogs::OpenFolder(const char* title)
	{
		BROWSEINFOA bi = { 0 };
		bi.hwndOwner = (HWND)Application::Get().GetWindow().GetWindowHandle();
		bi.lpszTitle = title;
		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;

		LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
		if (pidl != nullptr)
		{
			CHAR szPath[MAX_PATH];
			if (SHGetPathFromIDListA(pidl, szPath))
			{
				CoTaskMemFree(pidl);
				return std::filesystem::path(szPath);
			}
			CoTaskMemFree(pidl);
		}
		return std::filesystem::path();
	}

}