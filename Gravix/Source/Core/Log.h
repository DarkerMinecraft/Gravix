#pragma once

#include <memory>
#include <filesystem>

#include "Core.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Gravix
{
	class Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

		inline static bool IsActive() { return s_IsActive; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;

		static bool s_IsActive;
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

	// GLM vec2 formatter
	template <>
	struct fmt::formatter<glm::vec2> : fmt::formatter<std::string>
	{
		auto format(const glm::vec2& v, format_context& ctx) const
		{
			return fmt::format_to(ctx.out(), "vec2({:.3f}, {:.3f})", v.x, v.y);
		}
	};

	// GLM vec3 formatter
	template <>
	struct fmt::formatter<glm::vec3> : fmt::formatter<std::string>
	{
		auto format(const glm::vec3& v, format_context& ctx) const
		{
			return fmt::format_to(ctx.out(), "vec3({:.3f}, {:.3f}, {:.3f})", v.x, v.y, v.z);
		}
	};

	// GLM vec4 formatter
	template <>
	struct fmt::formatter<glm::vec4> : fmt::formatter<std::string>
	{
		auto format(const glm::vec4& v, format_context& ctx) const
		{
			return fmt::format_to(ctx.out(), "vec4({:.3f}, {:.3f}, {:.3f}, {:.3f})", v.x, v.y, v.z, v.w);
		}
	};

	// GLM mat2 formatter
	template <>
	struct fmt::formatter<glm::mat2> : fmt::formatter<std::string>
	{
		auto format(const glm::mat2& m, format_context& ctx) const
		{
			return fmt::format_to(ctx.out(), "mat2([{:.3f}, {:.3f}], [{:.3f}, {:.3f}])",
				m[0][0], m[0][1],
				m[1][0], m[1][1]);
		}
	};

	// GLM mat3 formatter
	template <>
	struct fmt::formatter<glm::mat3> : fmt::formatter<std::string>
	{
		auto format(const glm::mat3& m, format_context& ctx) const
		{
			return fmt::format_to(ctx.out(), "mat3([{:.3f}, {:.3f}, {:.3f}], [{:.3f}, {:.3f}, {:.3f}], [{:.3f}, {:.3f}, {:.3f}])",
				m[0][0], m[0][1], m[0][2],
				m[1][0], m[1][1], m[1][2],
				m[2][0], m[2][1], m[2][2]);
		}
	};

	// GLM mat4 formatter
	template <>
	struct fmt::formatter<glm::mat4> : fmt::formatter<std::string>
	{
		auto format(const glm::mat4& m, format_context& ctx) const
		{
			return fmt::format_to(ctx.out(), "mat4([{:.3f}, {:.3f}, {:.3f}, {:.3f}], [{:.3f}, {:.3f}, {:.3f}, {:.3f}], [{:.3f}, {:.3f}, {:.3f}, {:.3f}], [{:.3f}, {:.3f}, {:.3f}, {:.3f}])",
				m[0][0], m[0][1], m[0][2], m[0][3],
				m[1][0], m[1][1], m[1][2], m[1][3],
				m[2][0], m[2][1], m[2][2], m[2][3],
				m[3][0], m[3][1], m[3][2], m[3][3]);
		}
	};

	// GLM quat formatter
	template <>
	struct fmt::formatter<glm::quat> : fmt::formatter<std::string>
	{
		auto format(const glm::quat& q, format_context& ctx) const
		{
			return fmt::format_to(ctx.out(), "quat({:.3f}, {:.3f}, {:.3f}, {:.3f})", q.w, q.x, q.y, q.z);
		}
	};
}

#define GX_CORE_CRITICAL(...) if(::Gravix::Log::IsActive()) ::Gravix::Log::GetCoreLogger()->critical(__VA_ARGS__);
#define GX_CORE_ERROR(...) if(::Gravix::Log::IsActive()) ::Gravix::Log::GetCoreLogger()->error(__VA_ARGS__);
#define GX_CORE_WARN(...) if(::Gravix::Log::IsActive()) ::Gravix::Log::GetCoreLogger()->warn(__VA_ARGS__);
#define GX_CORE_INFO(...) if(::Gravix::Log::IsActive()) ::Gravix::Log::GetCoreLogger()->info(__VA_ARGS__);
#define GX_CORE_TRACE(...) if(::Gravix::Log::IsActive()) ::Gravix::Log::GetCoreLogger()->trace(__VA_ARGS__);

#define GX_ERROR(...) if(::Gravix::Log::IsActive()) ::Gravix::Log::GetClientLogger()->error(__VA_ARGS__);
#define GX_WARN(...) if(::Gravix::Log::IsActive()) ::Gravix::Log::GetClientLogger()->warn(__VA_ARGS__);
#define GX_INFO(...) if(::Gravix::Log::IsActive()) ::Gravix::Log::GetClientLogger()->info(__VA_ARGS__);
#define GX_TRACE(...) if(::Gravix::Log::IsActive()) ::Gravix::Log::GetClientLogger()->trace(__VA_ARGS__);