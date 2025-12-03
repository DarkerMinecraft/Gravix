#include "pch.h"
#include "FileWatcher.h"
#include "Core/Log.h"

namespace Gravix
{

	FileWatcher::~FileWatcher()
	{
		StopWatching();
	}

	void FileWatcher::StartWatching(const std::filesystem::path& directory, Callback callback)
	{
		if (m_IsWatching)
		{
			GX_CORE_WARN("FileWatcher is already watching. Stop first before starting a new watch.");
			return;
		}

		if (!std::filesystem::exists(directory))
		{
			GX_CORE_ERROR("FileWatcher: Directory does not exist: {0}", directory.string());
			return;
		}

		if (!std::filesystem::is_directory(directory))
		{
			GX_CORE_ERROR("FileWatcher: Path is not a directory: {0}", directory.string());
			return;
		}

		m_WatchPath = std::filesystem::absolute(directory);
		m_Callback = callback;
		m_IsWatching = true;

		// Initial scan to populate file modification times
		ScanDirectory();

		GX_CORE_INFO("FileWatcher: Started watching {0} (found {1} files)",
			m_WatchPath.string(), m_FileModTimes.size());
	}

	void FileWatcher::StopWatching()
	{
		if (!m_IsWatching)
			return;

		m_IsWatching = false;
		m_FileModTimes.clear();
		m_Callback = nullptr;
	}

	void FileWatcher::SetFileFilter(const std::string& filter)
	{
		m_FileFilter = filter;

		// Normalize filter to ensure it starts with a dot if it's an extension
		if (!m_FileFilter.empty() && m_FileFilter[0] != '.')
		{
			m_FileFilter = "." + m_FileFilter;
		}
	}

	void FileWatcher::CheckForChanges()
	{
		if (!m_IsWatching)
			return;

		try
		{
			// Scan directory for changes
			std::unordered_map<std::string, std::filesystem::file_time_type> currentFiles;

			for (const auto& entry : std::filesystem::recursive_directory_iterator(
				m_WatchPath,
				std::filesystem::directory_options::skip_permission_denied))
			{
				// Skip directories, only watch files
				if (!entry.is_regular_file())
					continue;

				const auto& path = entry.path();

				// Apply filter if set
				if (!PassesFilter(path))
					continue;

				std::string pathStr = path.string();
				auto lastWriteTime = std::filesystem::last_write_time(path);

				currentFiles[pathStr] = lastWriteTime;

				auto it = m_FileModTimes.find(pathStr);
				if (it == m_FileModTimes.end())
				{
					// New file detected
					if (m_Callback)
					{
						m_Callback(path, EventType::Added);
					}
				}
				else if (it->second != lastWriteTime)
				{
					// File modified
					if (m_Callback)
					{
						m_Callback(path, EventType::Modified);
					}
				}
			}

			// Check for removed files
			for (const auto& [pathStr, modTime] : m_FileModTimes)
			{
				if (currentFiles.find(pathStr) == currentFiles.end())
				{
					// File was removed
					if (m_Callback)
					{
						m_Callback(std::filesystem::path(pathStr), EventType::Removed);
					}
				}
			}

			// Update tracked files
			m_FileModTimes = std::move(currentFiles);
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			// Silently ignore filesystem errors (like permission denied)
			// These are common and expected in some directories
		}
		catch (const std::exception& e)
		{
			GX_CORE_ERROR("FileWatcher: Error checking for changes: {0}", e.what());
		}
	}

	void FileWatcher::ScanDirectory()
	{
		try
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(
				m_WatchPath,
				std::filesystem::directory_options::skip_permission_denied))
			{
				if (!entry.is_regular_file())
					continue;

				const auto& path = entry.path();

				if (!PassesFilter(path))
					continue;

				std::string pathStr = path.string();
				m_FileModTimes[pathStr] = std::filesystem::last_write_time(path);
			}
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			// Silently ignore filesystem errors during initial scan
		}
		catch (const std::exception& e)
		{
			GX_CORE_ERROR("FileWatcher: Error scanning directory: {0}", e.what());
		}
	}

	bool FileWatcher::PassesFilter(const std::filesystem::path& path) const
	{
		// No filter means accept all files
		if (m_FileFilter.empty())
			return true;

		// Check if extension matches filter
		return path.extension() == m_FileFilter;
	}

}
