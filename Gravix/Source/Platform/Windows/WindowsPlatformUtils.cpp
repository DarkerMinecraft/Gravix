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
		std::filesystem::path result;

		IFileDialog* pFileDialog = nullptr;
		HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pFileDialog));
		if (SUCCEEDED(hr))
		{
			DWORD options = 0;
			if (SUCCEEDED(pFileDialog->GetOptions(&options)))
				pFileDialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

			if (title)
			{
				// Convert title to wide string
				std::wstring wtitle(title, title + strlen(title));
				pFileDialog->SetTitle(wtitle.c_str());
			}

			hr = pFileDialog->Show((HWND)Application::Get().GetWindow().GetWindowHandle());
			if (SUCCEEDED(hr))
			{
				IShellItem* pItem = nullptr;
				if (SUCCEEDED(pFileDialog->GetResult(&pItem)))
				{
					PWSTR pszFolderPath = nullptr;
					if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFolderPath)))
					{
						char path[MAX_PATH];
						WideCharToMultiByte(CP_ACP, 0, pszFolderPath, -1, path, MAX_PATH, nullptr, nullptr);
						result = std::filesystem::path(path);
						CoTaskMemFree(pszFolderPath);
					}
					pItem->Release();
				}
			}

			pFileDialog->Release();
		}

		return result;
	}


}