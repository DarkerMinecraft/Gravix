#pragma once

#include "Core/Core.h"
#include "Core/FileWatcher.h"

#include <filesystem>
#include <mutex>
#include <chrono>

namespace Gravix
{

	/**
	 * @brief Watches C# script files and triggers reload when changes are detected
	 *
	 * Wraps the generic FileWatcher to provide script-specific functionality
	 * with debouncing and reload flag management.
	 */
	class ScriptFileWatcher
	{
	public:
		ScriptFileWatcher() = default;
		~ScriptFileWatcher();

		// Start watching script directories
		void StartWatching(const std::filesystem::path& scriptPath);
		void StopWatching();

		// Check for file changes (call from main thread)
		void CheckForChanges();

		// Check if reload is needed (call from main thread)
		bool ShouldReload() const { return m_NeedsReload; }
		void ClearReloadFlag() { m_NeedsReload = false; }

		// Get time since last change (for debouncing)
		int64_t GetMillisecondsSinceLastChange() const;

		bool IsWatching() const { return m_FileWatcher ? m_FileWatcher->IsWatching() : false; }

	private:
		void OnFileChanged(const std::filesystem::path& path, FileWatcher::EventType event);
		bool IsScriptFile(const std::filesystem::path& path) const;

	private:
		Scope<FileWatcher> m_FileWatcher;

		// Debouncing
		std::atomic<bool> m_NeedsReload = false;
		std::chrono::steady_clock::time_point m_LastChangeTime;
		mutable std::mutex m_ChangeMutex;
		static constexpr int DEBOUNCE_MS = 500; // Wait 500ms after last change
	};

}
