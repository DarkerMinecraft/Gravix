#include "pch.h"
#include "Console.h"
#include "Core/Log.h"

namespace Gravix
{

	std::vector<ConsoleMessage> Console::s_Messages;
	uint32_t Console::s_LogCount = 0;
	uint32_t Console::s_WarningCount = 0;
	uint32_t Console::s_ErrorCount = 0;

	void Console::Log(const std::string& message)
	{
		AddMessage(message, ConsoleMessageType::Log);
		s_LogCount++;
		GX_CORE_INFO("[Script] {0}", message);
	}

	void Console::LogWarning(const std::string& message)
	{
		AddMessage(message, ConsoleMessageType::Warning);
		s_WarningCount++;
		GX_CORE_WARN("[Script] {0}", message);
	}

	void Console::LogError(const std::string& message)
	{
		AddMessage(message, ConsoleMessageType::Error);
		s_ErrorCount++;
		GX_CORE_ERROR("[Script] {0}", message);
	}

	void Console::Clear()
	{
		s_Messages.clear();
		s_LogCount = 0;
		s_WarningCount = 0;
		s_ErrorCount = 0;
	}

	void Console::AddMessage(const std::string& message, ConsoleMessageType type)
	{
		// Check if the last message is the same (for message collapsing)
		if (!s_Messages.empty())
		{
			ConsoleMessage& lastMessage = s_Messages.back();
			if (lastMessage.Message == message && lastMessage.Type == type)
			{
				lastMessage.Count++;
				return;
			}
		}

		s_Messages.emplace_back(message, type);
	}

}
