#pragma once

#ifdef ENGINE_PLATFORM_WINDOWS
#include <Windows.h>
#include <string>

namespace Gravix
{
	/**
	 * @brief Simple splash screen window for editor startup
	 *
	 * Shows a splash screen with loading status during editor initialization.
	 * Uses native Win32 API for lightweight, immediate display.
	 */
	class SplashScreen
	{
	public:
		SplashScreen(const char* title = "Orbit Editor");
		~SplashScreen();

		void Show();
		void Hide();
		void Close();

		void SetStatus(const char* status);
		void SetProgress(float progress); // 0.0 to 1.0

		HWND GetWindowHandle() const { return m_Hwnd; }

	private:
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		void PaintWindow();

	private:
		HWND m_Hwnd = nullptr;
		HBITMAP m_BackBuffer = nullptr;
		HDC m_BackDC = nullptr;

		std::string m_Title;
		std::string m_Status = "Initializing...";
		float m_Progress = 0.0f;

		int m_Width = 600;
		int m_Height = 400;
	};
}

#endif // ENGINE_PLATFORM_WINDOWS
