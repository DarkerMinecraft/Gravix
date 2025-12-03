#pragma once

#include "Core/Core.h"
#include "Core/FileWatcher.h"
#include "Asset/Asset.h"

#include <filesystem>
#include <functional>
#include <unordered_set>
#include <mutex>

namespace Gravix
{

	enum class AssetWatchEvent
	{
		Added,
		Modified,
		Removed
	};

	struct AssetChangeInfo
	{
		std::filesystem::path FilePath;
		AssetWatchEvent Event;
		AssetType Type;
	};

	class AssetFileWatcher
	{
	public:
		AssetFileWatcher() = default;
		~AssetFileWatcher();

		// Start watching a directory
		void StartWatching(const std::filesystem::path& path);
		void StopWatching();

		// Check for file changes (call from main thread)
		void CheckForChanges();

		// Process pending changes (call from main thread)
		void ProcessChanges();

		// Register callback for asset changes
		using AssetChangeCallback = std::function<void(const AssetChangeInfo&)>;
		void SetChangeCallback(AssetChangeCallback callback) { m_ChangeCallback = callback; }

		bool IsWatching() const { return m_FileWatcher ? m_FileWatcher->IsWatching() : false; }

	private:
		void OnFileChanged(const std::filesystem::path& path, FileWatcher::EventType event);
		AssetType DetermineAssetType(const std::filesystem::path& path) const;
		bool IsAssetFile(const std::filesystem::path& path) const;

	private:
		Scope<FileWatcher> m_FileWatcher;
		AssetChangeCallback m_ChangeCallback;

		std::vector<AssetChangeInfo> m_PendingChanges;
		std::mutex m_ChangesMutex;

		// Debouncing - prevent duplicate events
		std::unordered_set<std::string> m_RecentChanges;
	};

}
