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

		std::vector<Ref<AsyncLoadRequest>> completedRequests;
		{
			GX_PROFILE_SCOPE("GatherCompletedRequests");
			std::lock_guard<std::mutex> lock(m_CompletionQueueMutex);
			while (!m_CompletionQueue.empty())
			{
				completedRequests.push_back(m_CompletionQueue.front());
				m_CompletionQueue.pop();
			}
		}

		bool registryChanged = false;

		{
			GX_PROFILE_SCOPE("ProcessCompletedRequests");
			for(Ref<AsyncLoadRequest> request : completedRequests)
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

}