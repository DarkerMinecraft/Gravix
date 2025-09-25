#include "pch.h"
#include "WindowsWindow.h"

#include "Events/WindowEvents.h"
#include "Events/KeyEvents.h"
#include "Events/MouseEvents.h"

#include "Core/Log.h"

#include "Renderer/Vulkan/VulkanDevice.h"

#include <windowsx.h>

#include <imgui.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Gravix
{

	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
			return true;

		if (msg == WM_CREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreate->lpCreateParams));
			return 0;
		}

		// CRITICAL FIX: Validate window data pointer before use
		LONG_PTR userData = GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (userData == 0) {
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		WindowData& pData = *(reinterpret_cast<WindowData*>(userData));

		switch (msg)
		{
		case WM_CLOSE:
		{
			WindowCloseEvent e;
			pData.EventCallback(e);
			return 0; // CRITICAL FIX: Return 0 to indicate we handled the message
		}

		// CRITICAL FIX: Handle window resize events more robustly
		case WM_SIZE:
		{
			uint32_t width = LOWORD(lParam);
			uint32_t height = HIWORD(lParam);

			WindowResizeEvent event(width, height);
			if (pData.EventCallback)
				pData.EventCallback(event);

			pData.Width = width;
			pData.Height = height;

			return 0;
		}

		// CRITICAL FIX: Handle window positioning changes that can affect rendering
		case WM_MOVE:
		{
			// Don't generate events for moves, but ensure the message is processed
			return 0;
		}

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			// CRITICAL FIX: Check for repeated key events
			int repeatCount = (lParam & 0x0000FFFF);
			bool wasDown = (lParam & 0x40000000) != 0;

			if (!wasDown || repeatCount == 1) {
				KeyPressedEvent event(static_cast<int>(wParam), repeatCount);
				pData.EventCallback(event);
			}
			return 0;
		}
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			KeyReleasedEvent event(static_cast<int>(wParam));
			pData.EventCallback(event);
			return 0;
		}
		case WM_CHAR:
		{
			// CRITICAL FIX: Filter out control characters
			if (wParam >= 32 && wParam < 127) {
				KeyTypedEvent event(static_cast<int>(wParam));
				pData.EventCallback(event);
			}
			return 0;
		}
		case WM_LBUTTONDOWN:
		{
			SetCapture(hwnd); // Capture mouse for proper tracking
			MouseButtonPressedEvent e(0);
			pData.EventCallback(e);
			return 0;
		}
		case WM_RBUTTONDOWN:
		{
			SetCapture(hwnd);
			MouseButtonPressedEvent e(1);
			pData.EventCallback(e);
			return 0;
		}
		case WM_MBUTTONDOWN:
		{
			SetCapture(hwnd);
			MouseButtonPressedEvent e(2);
			pData.EventCallback(e);
			return 0;
		}
		case WM_LBUTTONUP:
		{
			ReleaseCapture();
			MouseButtonReleasedEvent e(0);
			pData.EventCallback(e);
			return 0;
		}
		case WM_RBUTTONUP:
		{
			ReleaseCapture();
			MouseButtonReleasedEvent e(1);
			pData.EventCallback(e);
			return 0;
		}
		case WM_MBUTTONUP:
		{
			ReleaseCapture();
			MouseButtonReleasedEvent e(2);
			pData.EventCallback(e);
			return 0;
		}
		case WM_MOUSEWHEEL:
		{
			float xOffset = 0.0f;
			float yOffset = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);

			pData.ScrollX = 0.0f;
			pData.ScrollY = yOffset;

			MouseScrolledEvent e(xOffset, yOffset);
			pData.EventCallback(e);
			return 0;
		}
		case WM_MOUSEHWHEEL:
		{
			float xOffset = -static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
			float yOffset = 0.0f;

			pData.ScrollX = xOffset;
			pData.ScrollY = 0.0f;

			MouseScrolledEvent e(xOffset, yOffset);
			pData.EventCallback(e);
			return 0;
		}
		case WM_MOUSEMOVE:
		{
			// CRITICAL FIX: Use client coordinates instead of screen coordinates
			int xPos = GET_X_LPARAM(lParam);
			int yPos = GET_Y_LPARAM(lParam);

			MouseMovedEvent e(static_cast<float>(xPos), static_cast<float>(yPos));
			pData.EventCallback(e);
			return 0;
		}

		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
	}

	WindowsWindow::WindowsWindow(const WindowSpecification& spec)
	{
		Init(spec);
	}

	WindowsWindow::~WindowsWindow()
	{
		Destroy();
	}

	Scope<Window> Window::Create(const WindowSpecification& spec)
	{
		return CreateScope<WindowsWindow>(spec);
	}

	void WindowsWindow::OnUpdate()
	{
		m_Data.ScrollX = 0.0f;
		m_Data.ScrollY = 0.0f;

		MSG msg = {};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	void WindowsWindow::SetCursorMode(CursorMode mode)
	{

		// Ensure window has focus before changing cursor mode
		SetForegroundWindow(m_Window);
		SetFocus(m_Window);

		switch (mode)
		{
		case CursorMode::Normal:
		{
			// Remove cursor clipping first
			ClipCursor(NULL);

			// Show cursor - more reliable method
			int cursorCount = ShowCursor(TRUE);
			while (cursorCount < 0)
			{
				cursorCount = ShowCursor(TRUE);
			}

			// Force immediate update
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			break;
		}
		case CursorMode::Hidden:
		{
			// Remove cursor clipping
			ClipCursor(NULL);

			// Hide cursor - more reliable method
			int cursorCount = ShowCursor(FALSE);
			while (cursorCount >= 0)
			{
				cursorCount = ShowCursor(FALSE);
			}
			break;
		}
		case CursorMode::Disabled:
		{
			// Hide cursor first
			int cursorCount = ShowCursor(FALSE);
			while (cursorCount >= 0)
			{
				cursorCount = ShowCursor(FALSE);
			}

			// Get window rectangle in screen coordinates
			RECT windowRect;
			GetWindowRect(m_Window, &windowRect);

			// Center cursor in window BEFORE clipping
			int centerX = (windowRect.left + windowRect.right) / 2;
			int centerY = (windowRect.top + windowRect.bottom) / 2;
			SetCursorPos(centerX, centerY);

			// Now clip cursor to window bounds
			ClipCursor(&windowRect);

			// Verify cursor position was set correctly
			POINT cursorPos;
			GetCursorPos(&cursorPos);
			if (cursorPos.x != centerX || cursorPos.y != centerY)
			{
				SetCursorPos(centerX, centerY);
			}
			break;
		}
		}

		// Force Windows to process the cursor change immediately
		UpdateWindow(m_Window);

		// Process any pending messages to ensure immediate effect
		MSG msg;
		while (PeekMessage(&msg, m_Window, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	void WindowsWindow::Init(const WindowSpecification& spec)
	{
		m_Data.Width = spec.Width;
		m_Data.Height = spec.Height;
		m_Data.Title = spec.Title;

		HINSTANCE hInstance = GetModuleHandle(NULL);

		// Try to load the custom icon from resources
		HICON hAppIcon = nullptr;
		HICON hSmallIcon = nullptr;

		// Load large icon (32x32) from resources
		hAppIcon = static_cast<HICON>(LoadImage(hInstance, MAKEINTRESOURCE(101),
			IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR));

		// Load small icon (16x16) from resources  
		hSmallIcon = static_cast<HICON>(LoadImage(hInstance, MAKEINTRESOURCE(101),
			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));

		if (hAppIcon)
		{
			GX_CORE_INFO("Successfully loaded custom application icon from resources");
		}
		else
		{
			GX_CORE_TRACE("Custom icon not found in resources, using default");
			// Fall back to default application icon
			hAppIcon = LoadIcon(NULL, IDI_APPLICATION);
			hSmallIcon = LoadIcon(NULL, IDI_APPLICATION);
		}

		// Window class registration
		WNDCLASSEXA wc = {};
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.hInstance = hInstance;
		wc.hIcon = hAppIcon;        // Large icon (Alt+Tab, window title bar)
		wc.hIconSm = hSmallIcon;    // Small icon (taskbar, window title bar when minimized)
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;    // We don't need a background brush since Vulkan handles rendering
		wc.lpszClassName = "EngineWindowClass";

		if (!RegisterClassEx(&wc))
		{
			DWORD error = GetLastError();
			if (error != ERROR_CLASS_ALREADY_EXISTS)
			{
				GX_CORE_ERROR("Failed to register window class. Error: {}", error);
			}
		}

		// Get screen dimensions and position window
		if (m_Data.Width == -1 && m_Data.Height == -1)
		{
			m_Data.Width = GetSystemMetrics(SM_CXSCREEN);
			m_Data.Height = GetSystemMetrics(SM_CYSCREEN);
		}

		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);
		int posX = (screenWidth - m_Data.Width) / 2;
		int posY = (screenHeight - m_Data.Height) / 2;

		RECT windowRect = { 0, 0, static_cast<LONG>(m_Data.Width), static_cast<LONG>(m_Data.Height) };
		AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);

		// Create window
		m_Window = CreateWindowExA(
			WS_EX_APPWINDOW,
			"EngineWindowClass",
			m_Data.Title,
			WS_OVERLAPPEDWINDOW,
			posX, posY,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			nullptr, nullptr,
			hInstance,
			&m_Data
		);

		GX_CORE_ASSERT(m_Window, "Could not create Win32 window!");

		// Additional step: Set the icon for the window instance
		// This ensures the icon appears correctly in all contexts
		if (hAppIcon)
		{
			SendMessage(m_Window, WM_SETICON, ICON_BIG, (LPARAM)hAppIcon);
		}
		if (hSmallIcon)
		{
			SendMessage(m_Window, WM_SETICON, ICON_SMALL, (LPARAM)hSmallIcon);
		}

		ShowWindow(m_Window, SW_SHOW);

		m_Device = CreateScope<VulkanDevice>(DeviceProperties{ m_Data.Width, m_Data.Height, m_Window, false});
		UpdateWindow(m_Window);

		GX_CORE_INFO("Window created successfully with title: '{}'", m_Data.Title);
	}

	void WindowsWindow::Destroy()
	{
		if (m_Window == nullptr) return;

		DestroyWindow(m_Window);
		UnregisterClassA("EngineWindowClass", GetModuleHandle(NULL));

		GX_CORE_INFO("Window destroyed successfully");
		m_Window = nullptr;
	}

}