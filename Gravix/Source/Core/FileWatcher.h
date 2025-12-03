#pragma once

#include "Core/Core.h"

#include <filesystem>
#include <functional>
#include <unordered_map>
#include <string>
#include <chrono>

namespace Gravix
{

	/**
	 * @brief Cross-platform file watcher using std::filesystem polling
	 *
	 * Watches a directory recursively for file changes (added, modified, removed).
	 * Works on Windows, Linux, and macOS.
	 *
	 * Usage:
	 *   FileWatcher watcher;
	 *   watcher.SetFileFilter("*.cs");
	 *   watcher.StartWatching("path/to/watch", [](const auto& path, auto event) {
	 *       // Handle file change
	 *   });
	 *
	 *   // In your update loop:
	 *   watcher.CheckForChanges();
	 */
	class FileWatcher
	{
	public:
		enum class EventType
		{
			Added,
			Modified,
			Removed
		};

		using Callback = std::function<void(const std::filesystem::path& path, EventType event)>;

	public:
		FileWatcher() = default;
		~FileWatcher();

		/**
		 * @brief Start watching a directory for file changes
		 * @param directory Path to watch (will be watched recursively)
		 * @param callback Function to call when files change
		 */
		void StartWatching(const std::filesystem::path& directory, Callback callback);

		/**
		 * @brief Stop watching for file changes
		 */
		void StopWatching();

		/**
		 * @brief Check for file changes (call this periodically, e.g. every frame)
		 */
		void CheckForChanges();

		/**
		 * @brief Check if currently watching
		 */
		bool IsWatching() const { return m_IsWatching; }

		/**
		 * @brief Set file filter pattern (e.g., "*.cs", "*.txt", or empty for all files)
		 * @param filter Extension filter (e.g., ".cs") or empty for all files
		 */
		void SetFileFilter(const std::string& filter);

		/**
		 * @brief Get the path being watched
		 */
		const std::filesystem::path& GetWatchPath() const { return m_WatchPath; }

	private:
		void ScanDirectory();
		bool PassesFilter(const std::filesystem::path& path) const;

	private:
		std::filesystem::path m_WatchPath;
		std::unordered_map<std::string, std::filesystem::file_time_type> m_FileModTimes;
		Callback m_Callback;
		bool m_IsWatching = false;
		std::string m_FileFilter; // Extension filter (e.g., ".cs")
	};

}
