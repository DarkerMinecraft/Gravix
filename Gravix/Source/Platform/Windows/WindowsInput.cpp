#include "pch.h"
#include "Core/Input.h"
#include "Core/Application.h"

#include "WindowsWindow.h"

namespace Gravix
{

	bool Input::IsMouseDown(Mouse button)
	{
		return (GetAsyncKeyState(static_cast<int>(button)) & 0x8000) != 0;
	}

	bool Input::IsKeyDown(Key key)
	{
		return (GetAsyncKeyState(static_cast<int>(key)) & 0x8000) != 0;
	}

	bool Input::IsKeyPressed(Key key)
	{
		return (GetAsyncKeyState(static_cast<int>(key)) & 0x0001) != 0;
	}

	float Input::GetMouseX()
	{
		return GetMousePosition().x;
	}

	float Input::GetMouseY()
	{
		return GetMousePosition().y;
	}

	float Input::GetScrollX()
	{
		return GetScrollWheel().x;
	}

	float Input::GetScrollY()
	{
		return GetScrollWheel().y;
	}

	glm::vec2 Input::GetScrollWheel()
	{
		WindowsWindow& window = static_cast<WindowsWindow&>(Application::Get().GetWindow());
		auto& data = window.GetWindowData();

		return { data.ScrollX, data.ScrollY };
	}

	glm::vec2 Input::GetMousePosition()
	{
		HWND hwnd = static_cast<HWND>(Application::Get().GetWindow().GetWindowHandle());
		POINT p;

		GetCursorPos(&p);
		ScreenToClient(hwnd, &p);

		return { static_cast<float>(p.x), static_cast<float>(p.y) };
	}

}