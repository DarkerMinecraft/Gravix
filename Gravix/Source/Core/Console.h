#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace Gravix
{

	enum class ConsoleMessageType
	{
		Log = 0,
		Warning = 1,
		Error = 2
	};

	struct ConsoleMessage
	{
		std::string Message;
		ConsoleMessageType Type;
		std::chrono::system_clock::time_point Timestamp;
		uint32_t Count = 1; // For message collapsing

		ConsoleMessage(const std::string& message, ConsoleMessageType type)
			: Message(message), Type(type), Timestamp(std::chrono::system_clock::now())
		{
		}
	};

	class Console
	{
	public:
		static void Log(const std::string& message);
		static void LogWarning(const std::string& message);
		static void LogError(const std::string& message);

		static void Clear();

		static const std::vector<ConsoleMessage>& GetMessages() { return s_Messages; }

		// Statistics
		static uint32_t GetLogCount() { return s_LogCount; }
		static uint32_t GetWarningCount() { return s_WarningCount; }
		static uint32_t GetErrorCount() { return s_ErrorCount; }

	private:
		static void AddMessage(const std::string& message, ConsoleMessageType type);

	private:
		static std::vector<ConsoleMessage> s_Messages;
		static uint32_t s_LogCount;
		static uint32_t s_WarningCount;
		static uint32_t s_ErrorCount;
	};

}
