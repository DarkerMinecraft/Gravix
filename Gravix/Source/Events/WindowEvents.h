#pragma once

#include "Event.h"

#include <sstream>
#include <vector>

namespace Gravix
{

	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowCloseEvent";
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowClose)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(unsigned int width, unsigned int height)
			: m_Width(width), m_Height(height) {}

		inline unsigned int GetWidth() const { return m_Width; }
		inline unsigned int GetHeight() const { return m_Height; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowResize)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	private:
		unsigned int m_Width, m_Height;
	};

	class WindowFileDropEvent : public Event
	{
	public:
		WindowFileDropEvent(const std::vector<std::string>& paths)
			: m_Paths(paths) {}

		const std::vector<std::string>& GetPaths() const { return m_Paths; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowFileDropEvent: " << m_Paths.size() << " file(s)";
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowFileDrop)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	private:
		std::vector<std::string> m_Paths;
	};
}