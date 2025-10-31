#pragma once

// CRITICAL FIX: Prevent Windows API conflicts
#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// Exclude GDI functions that conflict with our enums
#define NOGDI
#endif

enum Mouse
{
	LeftButton = 0x01,      // VK_LBUTTON
	RightButton = 0x02,     // VK_RBUTTON  
	MiddleButton = 0x04     // VK_MBUTTON
};

enum Key
{
	// Alphabet keys (0x41-0x5A)
	A = 0x41, B = 0x42, C = 0x43, D = 0x44, E = 0x45, F = 0x46,
	G = 0x47, H = 0x48, I = 0x49, J = 0x4A, K = 0x4B, L = 0x4C,
	M = 0x4D, N = 0x4E, O = 0x4F, P = 0x50, Q = 0x51, R = 0x52,
	S = 0x53, T = 0x54, U = 0x55, V = 0x56, W = 0x57, X = 0x58,
	Y = 0x59, Z = 0x5A,

	// Number keys (0x30-0x39)
	Key0 = 0x30, Key1 = 0x31, Key2 = 0x32, Key3 = 0x33, Key4 = 0x34,
	Key5 = 0x35, Key6 = 0x36, Key7 = 0x37, Key8 = 0x38, Key9 = 0x39,

	// Function keys (0x70-0x7B)
	F1 = 0x70, F2 = 0x71, F3 = 0x72, F4 = 0x73, F5 = 0x74, F6 = 0x75,
	F7 = 0x76, F8 = 0x77, F9 = 0x78, F10 = 0x79, F11 = 0x7A, F12 = 0x7B,

	// Arrow keys
	Left = 0x25, Up = 0x26, Right = 0x27, Down = 0x28,

	// Special keys - FIXED: Use scoped name to avoid Windows API conflict
	Space = 0x20,
	Enter = 0x0D,
	Esc = 0x1B,  // RENAMED to avoid conflict with Windows GDI Escape function
	Tab = 0x09,
	Backspace = 0x08,
	Delete = 0x2E,
	Insert = 0x2D,
	Home = 0x24,
	End = 0x23,
	PageUp = 0x21,
	PageDown = 0x22,

	// Modifier keys
	LeftShift = 0xA0, RightShift = 0xA1,
	LeftControl = 0xA2, RightControl = 0xA3,
	LeftAlt = 0xA4, RightAlt = 0xA5,
	LeftWindows = 0x5B, RightWindows = 0x5C,

	// Numpad keys (0x60-0x69)
	Numpad0 = 0x60, Numpad1 = 0x61, Numpad2 = 0x62, Numpad3 = 0x63,
	Numpad4 = 0x64, Numpad5 = 0x65, Numpad6 = 0x66, Numpad7 = 0x67,
	Numpad8 = 0x68, Numpad9 = 0x69, NumpadMultiply = 0x6A,
	NumpadAdd = 0x6B, NumpadSubtract = 0x6D, NumpadDecimal = 0x6E,
	NumpadDivide = 0x6F, NumLock = 0x90,

	// Common punctuation (OEM keys)
	Semicolon = 0xBA, Plus = 0xBB, Comma = 0xBC, Minus = 0xBD,
	Period = 0xBE, Slash = 0xBF, Tilde = 0xC0, LeftBracket = 0xDB,
	Backslash = 0xDC, RightBracket = 0xDD, Quote = 0xDE,

	// Lock keys
	CapsLock = 0x14, ScrollLock = 0x91,

	// System keys
	PrintScreen = 0x2C, Pause = 0x13, Menu = 0x12,

	// Generic modifier keys
	Shift = 0x10, Control = 0x11, Alt = 0x12
};

#include <glm/glm.hpp>

namespace Gravix 
{

	class Input
	{
	public:
		static bool IsMouseDown(Mouse button);

		static bool IsKeyDown(Key key);
		static bool IsKeyPressed(Key key);

		static float GetMouseX();
		static float GetMouseY();

		static float GetScrollX();
		static float GetScrollY();

		static glm::vec2 GetScrollWheel();
		static glm::vec2 GetMousePosition();
	};

}