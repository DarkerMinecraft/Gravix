#include "pch.h"
#include "EditorAssetManager.h"

#include "AssetImporter.h"
#include "Project/Project.h"
#include "Core/Scheduler.h"
#include "Core/Application.h"
#include "Debug/Instrumentor.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace Gravix 
{

	bool EditorAssetManager::IsAssetHandleValid(AssetHandle handle) const
	{
		return handle != 0 && m_AssetRegistry.contains(handle);
	}

	AssetType EditorAssetManager::GetAssetType(AssetHandle handle) const
	{
		return IsAssetHandleValid(handle) ? m_AssetRegistry.at(handle).Type : AssetType::None;
	}

	void EditorAssetManager::PushToCompletionQueue(Ref<AsyncLoadRequest> request)
	{
		std::lock_guard<std::mutex> lock(m_CompletionQueueMutex);
		m_CompletionQueue.push(request);
	}

	void EditorAssetManager::ProcessAsyncLoads()
	{
		GX_PROFILE_FUNCTION();

		m_CompletedRequestsCache.clear();
		{
			GX_PROFILE_SCOPE("GatherCompletedRequests");
			std::lock_guard<std::mutex> lock(m_CompletionQueueMutex);
			while (!m_CompletionQueue.empty())
			{
				m_CompletedRequestsCache.push_back(m_CompletionQueue.front());
				m_CompletionQueue.pop();
			}
		}

		bool registryChanged = false;

		{
			GX_PROFILE_SCOPE("ProcessCompletedRequests");
			for(Ref<AsyncLoadRequest> request : m_CompletedRequestsCache)
			{
			if (request->State == AssetState::Failed)
			{
				GX_CORE_ERROR("Failed to load asset asynchronously: {0}", request->FilePath.string());
				m_LoadingAssets.erase(request->Handle);
				// No delete needed - Ref<> handles cleanup
				continue;
			}
			if (request->State == AssetState::ReadyForGPU)
			{
				AssetMetadata metadata = GetAssetMetadata(request->Handle);

				// If this is a scene, process dependencies first
				if (metadata.Type == AssetType::Scene)
				{
					if (auto* sceneData = std::get_if<AsyncLoadRequest::SceneData>(&request->CPUData))
					{
						// Queue dependencies for async loading
						for (AssetHandle depHandle : sceneData->Dependencies)
						{
							// Only load if not already loaded or loading
							if (!IsAssetLoaded(depHandle) && !m_LoadingAssets.contains(depHandle))
							{
								if (IsAssetHandleValid(depHandle))
								{
									AssetMetadata depMetadata = GetAssetMetadata(depHandle);

									Ref<AsyncLoadRequest> depRequest = CreateRef<AsyncLoadRequest>();
									depRequest->Handle = depHandle;
									depRequest->FilePath = depMetadata.FilePath;
									depRequest->State = AssetState::NotLoaded;
									depRequest->Priority = LoadPriority::High; // Dependencies get high priority

									m_LoadingAssets[depHandle] = depRequest;

									AsyncLoadTask* depLoadTask = new AsyncLoadTask(1); // enkiTS manages this
									depLoadTask->LoadRequests.push_back(depRequest);
									Application::Get().GetScheduler().GetTaskScheduler().AddTaskSetToPipe(depLoadTask);

									GX_CORE_INFO("Auto-loading scene dependency: {0}", depMetadata.FilePath.string());
								}
								else
								{
									GX_CORE_WARN("Scene dependency {0} not found in registry", static_cast<uint64_t>(depHandle));
								}
							}
						}
					}
				}

				Ref<Asset> asset = AssetImporter::ImportAsset(request->Handle, metadata);
				if (!asset)
				{
					GX_CORE_ERROR("Failed to import asset after async load: {0}", request->FilePath.string());
					m_LoadingAssets.erase(request->Handle);
					// No delete needed - Ref<> handles cleanup
					continue;
				}
				request->State = AssetState::Loaded;
				m_AssetRegistry[request->Handle] = metadata;
				m_LoadedAssets[request->Handle] = asset;
				m_LoadingAssets.erase(request->Handle);
				GX_CORE_INFO("Asynchronously loaded asset: {0}", request->FilePath.string());
				registryChanged = true;
				// No delete needed - Ref<> handles cleanup
			}
			}
		}

		// Only serialize the asset registry if it actually changed
		{
			GX_PROFILE_SCOPE("SerializeAssetRegistry");
			if (registryChanged)
			{
				SerializeAssetRegistry();
			}
		}
	}

	void EditorAssetManager::ImportAsset(const std::filesystem::path& filePath)
	{
		GX_PROFILE_FUNCTION();

		AssetMetadata metadata;
		AssetHandle handle = AssetImporter::GenerateAssetHandle(filePath, &metadata);

		Ref<AsyncLoadRequest> request = CreateRef<AsyncLoadRequest>();
		request->Handle = handle;
		request->FilePath = metadata.FilePath;
		request->State = AssetState::NotLoaded;
		request->Priority = LoadPriority::Normal;

		m_LoadingAssets[handle] = request;
		m_AssetRegistry[handle] = metadata;

		AsyncLoadTask* loadTask = new AsyncLoadTask(1); // enkiTS manages this
		loadTask->LoadRequests.push_back(request);
		Application::Get().GetScheduler().GetTaskScheduler().AddTaskSetToPipe(loadTask);
	}

	const AssetMetadata& EditorAssetManager::GetAssetMetadata(AssetHandle handle) const
	{
		static AssetMetadata invalidMetadata{};
		if(m_AssetRegistry.contains(handle))
			return m_AssetRegistry.at(handle);

		return invalidMetadata;
	}

	const std::filesystem::path& EditorAssetManager::GetAssetFilePath(AssetHandle handle) const
	{
		return GetAssetMetadata(handle).FilePath;
	}

	void EditorAssetManager::ClearLoadedAssets()
	{
		// Wait for GPU to finish using all assets before destroying them
		// This prevents destroying textures/resources that are still in use by command buffers
		Application::Get().GetWindow().GetDevice()->WaitIdle();

		m_LoadedAssets.clear();
	}

	void EditorAssetManager::SerializeAssetRegistry()
	{
		std::filesystem::path registryPath = Project::GetLibraryDirectory() / "AssetRegistry.orbreg";

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Assets" << YAML::Value << YAML::BeginSeq;
		for (const auto& [handle, metadata] : m_AssetRegistry)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Handle" << YAML::Value << static_cast<uint64_t>(handle);
			std::string filePathStr = metadata.FilePath.generic_string();
			out << YAML::Key << "FilePath" << YAML::Value << filePathStr;
			out << YAML::Key << "AssetType" << YAML::Value << AssetTypeToString(metadata.Type);
			out << YAML::Key << "LastModifiedTime" << YAML::Value << metadata.LastModifiedTime;
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;

		std::ofstream fout(registryPath);
		fout << out.c_str();
	}

	void EditorAssetManager::DeserializeAssetRegistry()
	{
		std::filesystem::path registryPath = Project::GetLibraryDirectory() / "AssetRegistry.orbreg";
		if (!std::filesystem::exists(registryPath)) 
		{
			GX_CORE_WARN("Asset registry file does not exist: {0}", registryPath.string());
			return;
		}

		std::ifstream stream(registryPath);
		YAML::Node data = YAML::Load(stream);

		YAML::Node assetsNode = data["Assets"];
		if (assetsNode)
		{
			for (const auto& assetNode : assetsNode)
			{
				AssetHandle handle = assetNode["Handle"].as<uint64_t>();
				std::filesystem::path filePath = assetNode["FilePath"].as<std::string>();
				AssetType type = StringToAssetType(assetNode["AssetType"].as<std::string>());
				uint64_t lastModifiedTime = assetNode["LastModifiedTime"].as<uint64_t>();
				AssetMetadata metadata;
				metadata.FilePath = filePath;
				metadata.Type = type;
				metadata.LastModifiedTime = lastModifiedTime;

				m_AssetRegistry[handle] = metadata;
			}
		}
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle handle) const
	{
		return m_LoadedAssets.contains(handle);
	}

	Ref<Asset> EditorAssetManager::GetAsset(AssetHandle handle)
	{
		if(!IsAssetHandleValid(handle))
			return nullptr;

		AssetMetadata metadata = GetAssetMetadata(handle);
		if(m_LoadingAssets.contains(handle))
		{
			GX_CORE_INFO("Asset is still loading: {0}", metadata.FilePath.string());
			return nullptr; // Asset is still loading
		}

		if (IsAssetLoaded(handle))
		{
			return m_LoadedAssets.at(handle);
		}

		Ref<AsyncLoadRequest> request = CreateRef<AsyncLoadRequest>();
		request->Handle = handle;
		request->FilePath = metadata.FilePath;
		request->State = AssetState::NotLoaded;
		request->Priority = LoadPriority::Normal;

		m_LoadingAssets[handle] = request;
		m_AssetRegistry[handle] = metadata;

		AsyncLoadTask* loadTask = new AsyncLoadTask(1); // enkiTS manages this
		loadTask->LoadRequests.push_back(request);
		Application::Get().GetScheduler().GetTaskScheduler().AddTaskSetToPipe(loadTask);
		return nullptr;
	}

	// File Watching Implementation

	void EditorAssetManager::StartWatchingAssets(const std::filesystem::path& assetPath)
	{
		if (!m_FileWatcher)
			m_FileWatcher = CreateScope<AssetFileWatcher>();

		m_FileWatcher->SetChangeCallback([this](const AssetChangeInfo& changeInfo) {
			OnAssetChanged(changeInfo);
		});

		m_FileWatcher->StartWatching(assetPath);
		GX_CORE_INFO("Asset file watcher started for: {0}", assetPath.string());
	}

	void EditorAssetManager::StopWatchingAssets()
	{
		if (m_FileWatcher)
		{
			m_FileWatcher->StopWatching();
			m_FileWatcher.reset();
		}
	}

	void EditorAssetManager::ProcessAssetChanges()
	{
		if (m_FileWatcher)
		{
			// Check for file changes (polling)
			m_FileWatcher->CheckForChanges();
			// Process pending changes
			m_FileWatcher->ProcessChanges();
		}
	}

	void EditorAssetManager::OnAssetChanged(const AssetChangeInfo& changeInfo)
	{
		// Find asset handle for this file path
		AssetHandle changedHandle = 0;
		for (const auto& [handle, metadata] : m_AssetRegistry)
		{
			if (metadata.FilePath == changeInfo.FilePath)
			{
				changedHandle = handle;
				break;
			}
		}

		switch (changeInfo.Event)
		{
		case AssetWatchEvent::Modified:
		{
			if (changedHandle != 0)
			{
				GX_CORE_INFO("Asset modified: {0}", changeInfo.FilePath.filename().string());
				ReloadAsset(changedHandle);
			}
			break;
		}
		case AssetWatchEvent::Removed:
		{
			if (changedHandle != 0)
			{
				GX_CORE_INFO("Asset removed: {0}", changeInfo.FilePath.filename().string());
				UnloadAsset(changedHandle);
				m_AssetRegistry.erase(changedHandle);
			}
			break;
		}
		case AssetWatchEvent::Added:
		{
			GX_CORE_INFO("Asset added: {0}", changeInfo.FilePath.filename().string());
			// Import the new asset
			ImportAsset(changeInfo.FilePath);
			break;
		}
		}
	}

	void EditorAssetManager::ReloadAsset(AssetHandle handle)
	{
		// Check if asset is loaded
		auto it = m_LoadedAssets.find(handle);
		if (it == m_LoadedAssets.end())
		{
			// Asset not loaded, nothing to reload
			return;
		}

		// Get asset metadata
		const auto& metadata = GetAssetMetadata(handle);

		GX_CORE_INFO("Reloading asset: {0} ({1})", metadata.FilePath.filename().string(), AssetTypeToString(metadata.Type));

		// Unload old asset (this will destroy Vulkan resources)
		UnloadAsset(handle);

		// Wait a bit for GPU operations to complete
		// TODO: Better synchronization with rendering system
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		// Reload asset asynchronously
		GetAsset(handle);
	}

	void EditorAssetManager::UnloadAsset(AssetHandle handle)
	{
		auto it = m_LoadedAssets.find(handle);
		if (it != m_LoadedAssets.end())
		{
			// Asset destructor will clean up Vulkan resources
			// (Texture, Material, Mesh all have proper destructors)
			m_LoadedAssets.erase(it);
			GX_CORE_INFO("Asset unloaded: {0}", (uint64_t)handle);
		}
	}

}