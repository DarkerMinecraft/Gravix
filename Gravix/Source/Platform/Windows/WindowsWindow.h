#pragma once

#include "Core/Window.h"

#include <windows.h>

namespace Gravix
{

	struct WindowData
	{
		const char* Title;
		uint32_t Width, Height;

		EventCallbackFn EventCallback;

		float ScrollX, ScrollY;
	};

	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowSpecification& spec);
		virtual ~WindowsWindow();

		virtual void OnUpdate() override;

		virtual uint32_t GetWidth() override { return m_Data.Width; }
		virtual uint32_t GetHeight() override { return m_Data.Height; }

		virtual void SetCursorMode(CursorMode mode) override;

		virtual void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }

		virtual void* GetWindowHandle() override { return m_Window; }

		virtual Device* GetDevice() override { return m_Device.get(); }

		WindowData& GetWindowData() { return m_Data; }
	private:
		void Init(const WindowSpecification& spec);
		void Destroy();
	private:
		HWND m_Window;
		WindowData m_Data;

		Scope<Device> m_Device;
	};

}