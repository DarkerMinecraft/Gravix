#pragma once

#include "Events/Event.h"

namespace Gravix 
{

	struct WindowSpecification
	{
		uint32_t Width;
		uint32_t Height;
		const char* Title;

		WindowSpecification(uint32_t width = 1280, uint32_t height = 720, const char* title = "Gravix Engine")
			: Width(width), Height(height), Title(title) {}
	};

	using EventCallbackFn = std::function<void(Event&)>;

	enum CursorMode
	{
		Normal,
		Hidden,
		Disabled
	};

	class Window 
	{
	public:
		public:
			virtual ~Window() = default;

			static Scope<Window> Create(const WindowSpecification& spec);

			virtual void OnUpdate() = 0;

			virtual uint32_t GetWidth() = 0;
			virtual uint32_t GetHeight() = 0;

			virtual void SetCursorMode(CursorMode mode) = 0;

			virtual void SetEventCallback(const EventCallbackFn& callback) = 0;

			virtual void* GetWindowHandle() = 0;
	};

}