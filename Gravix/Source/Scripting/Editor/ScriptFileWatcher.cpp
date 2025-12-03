#include "pch.h"
#include "ScriptFileWatcher.h"
#include "Core/Log.h"

namespace Gravix
{

	ScriptFileWatcher::~ScriptFileWatcher()
	{
		StopWatching();
	}

	void ScriptFileWatcher::StartWatching(const std::filesystem::path& scriptPath)
	{
		if (m_FileWatcher && m_FileWatcher->IsWatching())
		{
			GX_CORE_WARN("ScriptFileWatcher is already watching. Stop first.");
			return;
		}

		// Create file watcher
		m_FileWatcher = CreateScope<FileWatcher>();
		m_FileWatcher->SetFileFilter(".cs"); // Only watch C# files

		// Start watching with callback
		m_FileWatcher->StartWatching(scriptPath, [this](const std::filesystem::path& path, FileWatcher::EventType event) {
			OnFileChanged(path, event);
		});

		m_LastChangeTime = std::chrono::steady_clock::now();
	}

	void ScriptFileWatcher::StopWatching()
	{
		if (!m_FileWatcher)
			return;

		m_FileWatcher->StopWatching();
		m_FileWatcher.reset();
		m_NeedsReload = false;
	}

	void ScriptFileWatcher::CheckForChanges()
	{
		if (!m_FileWatcher)
			return;

		m_FileWatcher->CheckForChanges();
	}

	void ScriptFileWatcher::OnFileChanged(const std::filesystem::path& path, FileWatcher::EventType event)
	{
		// Apply additional filtering for script files
		if (!IsScriptFile(path))
			return;

		std::lock_guard<std::mutex> lock(m_ChangeMutex);
		m_LastChangeTime = std::chrono::steady_clock::now();
		m_NeedsReload = true;

		const char* eventStr = "";
		switch (event)
		{
		case FileWatcher::EventType::Added:    eventStr = "Added"; break;
		case FileWatcher::EventType::Modified: eventStr = "Modified"; break;
		case FileWatcher::EventType::Removed:  eventStr = "Removed"; break;
		default: eventStr = "Unknown"; break;
		}

		GX_CORE_INFO("ScriptFileWatcher: {0} - {1} (reload pending)", eventStr, path.filename().string());
	}

	int64_t ScriptFileWatcher::GetMillisecondsSinceLastChange() const
	{
		std::lock_guard<std::mutex> lock(m_ChangeMutex);
		auto now = std::chrono::steady_clock::now();
		return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastChangeTime).count();
	}

	bool ScriptFileWatcher::IsScriptFile(const std::filesystem::path& path) const
	{
		// Check extension
		if (path.extension() != ".cs")
			return false;

		// Ignore hidden/system files
		std::string filename = path.filename().string();
		if (filename.empty() || filename[0] == '.')
			return false;

		// Ignore temp files
		if (filename.find("~") != std::string::npos)
			return false;

		// Ignore obj/bin directories
		std::string pathStr = path.string();
		if (pathStr.find("\\obj\\") != std::string::npos ||
			pathStr.find("/obj/") != std::string::npos ||
			pathStr.find("\\bin\\") != std::string::npos ||
			pathStr.find("/bin/") != std::string::npos)
		{
			return false;
		}

		return true;
	}

}
