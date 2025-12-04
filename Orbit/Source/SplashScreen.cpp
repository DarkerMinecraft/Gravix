#include "SplashScreen.h"

#ifdef ENGINE_PLATFORM_WINDOWS

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

namespace Gravix
{
	static const wchar_t* SPLASH_WINDOW_CLASS = L"OrbitSplashWindow";

	SplashScreen::SplashScreen(const char* title)
		: m_Title(title)
	{
		// Register window class
		WNDCLASSEXW wc = {};
		wc.cbSize = sizeof(WNDCLASSEXW);
		wc.lpfnWndProc = WindowProc;
		wc.hInstance = GetModuleHandle(nullptr);
		wc.lpszClassName = SPLASH_WINDOW_CLASS;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);

		RegisterClassExW(&wc);

		// Calculate centered position
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);
		int x = (screenWidth - m_Width) / 2;
		int y = (screenHeight - m_Height) / 2;

		// Create borderless window with rounded corners
		m_Hwnd = CreateWindowExW(
			WS_EX_TOPMOST | WS_EX_LAYERED,
			SPLASH_WINDOW_CLASS,
			L"Orbit Editor",
			WS_POPUP,
			x, y, m_Width, m_Height,
			nullptr, nullptr,
			GetModuleHandle(nullptr),
			this
		);

		// Enable rounded corners (Windows 11)
		DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
		DwmSetWindowAttribute(m_Hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));

		// Create back buffer for double buffering
		HDC hdc = GetDC(m_Hwnd);
		m_BackDC = CreateCompatibleDC(hdc);
		m_BackBuffer = CreateCompatibleBitmap(hdc, m_Width, m_Height);
		SelectObject(m_BackDC, m_BackBuffer);
		ReleaseDC(m_Hwnd, hdc);

		// Set window opacity
		SetLayeredWindowAttributes(m_Hwnd, 0, 250, LWA_ALPHA);
	}

	SplashScreen::~SplashScreen()
	{
		Close();

		if (m_BackBuffer)
		{
			DeleteObject(m_BackBuffer);
			m_BackBuffer = nullptr;
		}

		if (m_BackDC)
		{
			DeleteDC(m_BackDC);
			m_BackDC = nullptr;
		}
	}

	void SplashScreen::Show()
	{
		if (m_Hwnd)
		{
			ShowWindow(m_Hwnd, SW_SHOW);
			UpdateWindow(m_Hwnd);
			PaintWindow();
		}
	}

	void SplashScreen::Hide()
	{
		if (m_Hwnd)
		{
			ShowWindow(m_Hwnd, SW_HIDE);
		}
	}

	void SplashScreen::Close()
	{
		if (m_Hwnd)
		{
			DestroyWindow(m_Hwnd);
			m_Hwnd = nullptr;
		}

		UnregisterClassW(SPLASH_WINDOW_CLASS, GetModuleHandle(nullptr));
	}

	void SplashScreen::SetStatus(const char* status)
	{
		m_Status = status;
		PaintWindow();

		// Process messages to keep UI responsive
		MSG msg;
		while (PeekMessage(&msg, m_Hwnd, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	void SplashScreen::SetProgress(float progress)
	{
		m_Progress = progress;
		PaintWindow();
	}

	void SplashScreen::PaintWindow()
	{
		if (!m_Hwnd || !m_BackDC)
			return;

		// Clear background with dark theme
		HBRUSH bgBrush = CreateSolidBrush(RGB(30, 30, 30));
		RECT rect = { 0, 0, m_Width, m_Height };
		FillRect(m_BackDC, &rect, bgBrush);
		DeleteObject(bgBrush);

		// Draw title
		SetTextColor(m_BackDC, RGB(255, 255, 255));
		SetBkMode(m_BackDC, TRANSPARENT);

		HFONT titleFont = CreateFontW(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
		HFONT oldFont = (HFONT)SelectObject(m_BackDC, titleFont);

		RECT titleRect = { 0, 80, m_Width, 140 };
		DrawTextW(m_BackDC, L"Orbit Editor", -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		SelectObject(m_BackDC, oldFont);
		DeleteObject(titleFont);

		// Draw status text
		HFONT statusFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
		SelectObject(m_BackDC, statusFont);

		SetTextColor(m_BackDC, RGB(180, 180, 180));
		RECT statusRect = { 50, m_Height - 120, m_Width - 50, m_Height - 100 };

		// Convert status to wide string
		int size = MultiByteToWideChar(CP_UTF8, 0, m_Status.c_str(), -1, nullptr, 0);
		wchar_t* wStatus = new wchar_t[size];
		MultiByteToWideChar(CP_UTF8, 0, m_Status.c_str(), -1, wStatus, size);

		DrawTextW(m_BackDC, wStatus, -1, &statusRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		delete[] wStatus;

		DeleteObject(statusFont);

		// Draw progress bar
		int barWidth = m_Width - 100;
		int barHeight = 8;
		int barX = 50;
		int barY = m_Height - 80;

		// Background bar
		HBRUSH barBgBrush = CreateSolidBrush(RGB(50, 50, 50));
		RECT barBgRect = { barX, barY, barX + barWidth, barY + barHeight };
		FillRect(m_BackDC, &barBgRect, barBgBrush);
		DeleteObject(barBgBrush);

		// Progress bar (accent color)
		HBRUSH progressBrush = CreateSolidBrush(RGB(0, 120, 212));
		int progressWidth = (int)(barWidth * m_Progress);
		RECT progressRect = { barX, barY, barX + progressWidth, barY + barHeight };
		FillRect(m_BackDC, &progressRect, progressBrush);
		DeleteObject(progressBrush);

		// Copy back buffer to window
		HDC hdc = GetDC(m_Hwnd);
		BitBlt(hdc, 0, 0, m_Width, m_Height, m_BackDC, 0, 0, SRCCOPY);
		ReleaseDC(m_Hwnd, hdc);
	}

	LRESULT CALLBACK SplashScreen::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		SplashScreen* splash = nullptr;

		if (uMsg == WM_NCCREATE)
		{
			CREATESTRUCTW* pCreate = (CREATESTRUCTW*)lParam;
			splash = (SplashScreen*)pCreate->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)splash);
		}
		else
		{
			splash = (SplashScreen*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		}

		if (splash)
		{
			switch (uMsg)
			{
			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				BeginPaint(hwnd, &ps);
				splash->PaintWindow();
				EndPaint(hwnd, &ps);
				return 0;
			}
			case WM_ERASEBKGND:
				return 1; // Prevent flicker
			}
		}

		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
}

#endif // ENGINE_PLATFORM_WINDOWS
