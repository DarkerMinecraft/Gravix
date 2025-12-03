#include "pch.h"
#include "AssetFileWatcher.h"
#include "Core/Log.h"

namespace Gravix
{

	AssetFileWatcher::~AssetFileWatcher()
	{
		StopWatching();
	}

	void AssetFileWatcher::StartWatching(const std::filesystem::path& path)
	{
		if (m_FileWatcher && m_FileWatcher->IsWatching())
		{
			GX_CORE_WARN("AssetFileWatcher is already watching a directory. Stop first.");
			return;
		}

		// Create file watcher (no filter - watch all files)
		m_FileWatcher = CreateScope<FileWatcher>();
		m_FileWatcher->StartWatching(path, [this](const std::filesystem::path& path, FileWatcher::EventType event) {
			OnFileChanged(path, event);
		});
	}

	void AssetFileWatcher::StopWatching()
	{
		if (!m_FileWatcher)
			return;

		m_FileWatcher->StopWatching();
		m_FileWatcher.reset();
		m_PendingChanges.clear();
		m_RecentChanges.clear();
	}

	void AssetFileWatcher::CheckForChanges()
	{
		if (!m_FileWatcher)
			return;

		m_FileWatcher->CheckForChanges();
	}

	void AssetFileWatcher::ProcessChanges()
	{
		if (!m_ChangeCallback)
			return;

		std::vector<AssetChangeInfo> changes;

		{
			std::lock_guard<std::mutex> lock(m_ChangesMutex);
			if (m_PendingChanges.empty())
				return;

			changes = std::move(m_PendingChanges);
			m_PendingChanges.clear();
		}

		// Process changes on main thread
		for (const auto& change : changes)
		{
			m_ChangeCallback(change);
		}

		// Clear recent changes after processing
		m_RecentChanges.clear();
	}

	void AssetFileWatcher::OnFileChanged(const std::filesystem::path& path, FileWatcher::EventType event)
	{
		// Ignore non-asset files
		if (!IsAssetFile(path))
			return;

		// Debouncing - ignore if we recently processed this file
		std::string pathStr = path.string();
		if (m_RecentChanges.find(pathStr) != m_RecentChanges.end())
			return;

		m_RecentChanges.insert(pathStr);

		AssetWatchEvent watchEvent;
		switch (event)
		{
		case FileWatcher::EventType::Added:
			watchEvent = AssetWatchEvent::Added;
			break;
		case FileWatcher::EventType::Modified:
			watchEvent = AssetWatchEvent::Modified;
			break;
		case FileWatcher::EventType::Removed:
			watchEvent = AssetWatchEvent::Removed;
			break;
		default:
			return;
		}

		AssetType type = DetermineAssetType(path);

		AssetChangeInfo changeInfo;
		changeInfo.FilePath = path;
		changeInfo.Event = watchEvent;
		changeInfo.Type = type;

		{
			std::lock_guard<std::mutex> lock(m_ChangesMutex);
			m_PendingChanges.push_back(changeInfo);
		}

		GX_CORE_INFO("AssetFileWatcher: {0} - {1}",
			watchEvent == AssetWatchEvent::Added ? "Added" :
			watchEvent == AssetWatchEvent::Modified ? "Modified" : "Removed",
			path.string());
	}

	AssetType AssetFileWatcher::DetermineAssetType(const std::filesystem::path& path) const
	{
		std::string extension = path.extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

		// Image formats
		if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
			extension == ".bmp" || extension == ".tga" || extension == ".hdr")
		{
			return AssetType::Texture2D;
		}

		// Scene formats
		if (extension == ".gxscene" || extension == ".scene")
		{
			return AssetType::Scene;
		}

		// Shader formats
		if (extension == ".slang" || extension == ".hlsl" || extension == ".glsl" ||
			extension == ".vert" || extension == ".frag" || extension == ".comp")
		{
			return AssetType::Shader;
		}

		// Material formats
		if (extension == ".mat" || extension == ".material")
		{
			return AssetType::Material;
		}

		// Note: .cs files are NOT assets - they're source code handled by the script system
		// They should not be added to the Asset Registry

		return AssetType::None;
	}

	bool AssetFileWatcher::IsAssetFile(const std::filesystem::path& path) const
	{
		// Ignore system/hidden files
		std::string filename = path.filename().string();
		if (filename.empty() || filename[0] == '.')
			return false;

		// Ignore temp files
		if (filename.find("~") != std::string::npos)
			return false;

		// Check if it's a recognized asset type
		return DetermineAssetType(path) != AssetType::None;
	}

}
