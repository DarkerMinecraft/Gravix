#pragma once

#include "Event.h"
#include <sstream>

namespace Gravix
{
	class KeyPressedEvent : public Event
	{
	public:
		KeyPressedEvent(int keycode, int repeatCount)
			: m_KeyCode(keycode), m_RepeatCount(repeatCount)
		{}

		inline int GetKeyCode() const { return m_KeyCode; }
		inline int GetRepeatCount() const { return m_RepeatCount; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyPressedEvent: " << m_KeyCode << " (" << m_RepeatCount << " repeats)";
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyPressed)
		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
	private:
		int m_KeyCode;
		int m_RepeatCount;
	};

	class KeyReleasedEvent : public Event
	{
	public:
		KeyReleasedEvent(int keycode)
			: m_KeyCode(keycode)
		{}

		inline int GetKeyCode() const { return m_KeyCode; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyReleased)
		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
	private:
		int m_KeyCode;
	};

	class KeyTypedEvent : public Event
	{
	public:
		KeyTypedEvent(int keycode)
			: m_KeyCode(keycode)
		{}

		inline int GetKeyCode() const { return m_KeyCode; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyTypedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyTyped)
		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
	private:
		int m_KeyCode;
	};
}
