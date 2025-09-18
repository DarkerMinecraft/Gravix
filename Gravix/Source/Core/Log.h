#pragma once

#include <memory>
#include <filesystem>

#include "Core.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

namespace Gravix
{
	class Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

namespace fmt
{
	template <>
	struct fmt::formatter<std::filesystem::path> : fmt::formatter<std::string>
	{
		auto format(const std::filesystem::path& path, format_context& ctx) const
		{
			return formatter<std::string>::format(path.string(), ctx);
		}
	};

	template <>
	struct fmt::formatter<std::filesystem::directory_entry> : fmt::formatter<std::string>
	{
		auto format(const std::filesystem::directory_entry& entry, format_context& ctx) const
		{
			return formatter<std::string>::format(entry.path().string(), ctx);
		}
	};

	template <>
	struct fmt::formatter<std::filesystem::file_status> : fmt::formatter<std::string>
	{
		auto format(const std::filesystem::file_status& status, format_context& ctx) const
		{
			std::string type_str;
			switch (status.type()) {
			case std::filesystem::file_type::regular: type_str = "regular file"; break;
			case std::filesystem::file_type::directory: type_str = "directory"; break;
			case std::filesystem::file_type::symlink: type_str = "symlink"; break;
			case std::filesystem::file_type::block: type_str = "block device"; break;
			case std::filesystem::file_type::character: type_str = "character device"; break;
			case std::filesystem::file_type::fifo: type_str = "FIFO"; break;
			case std::filesystem::file_type::socket: type_str = "socket"; break;
			case std::filesystem::file_type::none: type_str = "none"; break;
			case std::filesystem::file_type::not_found: type_str = "not found"; break;
			default: type_str = "unknown"; break;
			}
			return formatter<std::string>::format(type_str, ctx);
		}
	};
}

#define EN_CORE_CRITICAL(...) ::Gravix::Log::GetCoreLogger()->critical(__VA_ARGS__);
#define EN_CORE_ERROR(...) ::Gravix::Log::GetCoreLogger()->error(__VA_ARGS__);
#define EN_CORE_WARN(...) ::Gravix::Log::GetCoreLogger()->warn(__VA_ARGS__);
#define EN_CORE_INFO(...) ::Gravix::Log::GetCoreLogger()->info(__VA_ARGS__);
#define EN_CORE_TRACE(...) ::Gravix::Log::GetCoreLogger()->trace(__VA_ARGS__);

#define EN_ERROR(...) ::Gravix::Log::GetClientLogger()->error(__VA_ARGS__);
#define EN_WARN(...) ::Gravix::Log::GetClientLogger()->warn(__VA_ARGS__);
#define EN_INFO(...) ::Gravix::Log::GetClientLogger()->info(__VA_ARGS__);
#define EN_TRACE(...) ::Gravix::Log::GetClientLogger()->trace(__VA_ARGS__);