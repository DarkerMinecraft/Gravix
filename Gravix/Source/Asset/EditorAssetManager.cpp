#include "pch.h"
#include "EditorAssetManager.h"

#include "AssetImporter.h"
#include "Project/Project.h"

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

	void EditorAssetManager::PushToCompletionQueue(AsyncLoadRequest* request)
	{
		std::lock_guard<std::mutex> lock(m_CompletionQueueMutex);
		m_CompletionQueue.push(request);
	}

	void EditorAssetManager::ProcessAsyncLoads()
	{
		std::vector<AsyncLoadRequest*> completedRequests;
		{
			std::lock_guard<std::mutex> lock(m_CompletionQueueMutex);
			while (!m_CompletionQueue.empty())
			{
				completedRequests.push_back(m_CompletionQueue.front());
				m_CompletionQueue.pop();
			}
		}

		for(AsyncLoadRequest* request : completedRequests)
		{
			if (request->State == AssetState::Failed)
			{
				GX_CORE_ERROR("Failed to load asset asynchronously: {0}", request->FilePath.string());
				continue;
			}
			if (request->State == AssetState::ReadyForGPU) 
			{
				AssetMetadata metadata = GetAssetMetadata(request->Handle);
				Ref<Asset> asset = AssetImporter::ImportAsset(request->Handle, metadata);
				if (!asset)
				{
					GX_CORE_ERROR("Failed to import asset after async load: {0}", request->FilePath.string());
					continue;
				}
				request->State = AssetState::Loaded;
				m_AssetRegistry[request->Handle] = metadata;
				m_LoadedAssets[request->Handle] = asset;
				m_LoadingAssets.erase(request->Handle);
				GX_CORE_INFO("Asynchronously loaded asset: {0}", request->FilePath.string());
			}
		}

		SerializeAssetRegistry();
	}

	void EditorAssetManager::ImportAsset(const std::filesystem::path& filePath)
	{
		AssetMetadata metadata;
		AssetHandle handle = AssetImporter::GenerateAssetHandle(filePath, &metadata);

		AsyncLoadRequest* request = new AsyncLoadRequest();
		request->Handle = handle;
		request->FilePath = metadata.FilePath;
		request->State = AssetState::NotLoaded;
		request->Priority = LoadPriority::Normal;

		m_LoadingAssets[handle] = request;
		m_AssetRegistry[handle] = metadata;
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
				AssetMetadata metadata;
				metadata.FilePath = filePath;
				metadata.Type = type;

				m_AssetRegistry[handle] = metadata;
			}
		}
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle handle) const
	{
		return m_LoadedAssets.contains(handle);
	}

	Ref<Asset> EditorAssetManager::GetAsset(AssetHandle handle) const
	{
		if(!IsAssetHandleValid(handle))
			return nullptr;

		Ref<Asset> asset;
		if(m_LoadingAssets.contains(handle)) 
		{
			GX_CORE_INFO("Asset is still loading: {0}", GetAssetMetadata(handle).FilePath.string());
			return nullptr; // Asset is still loading
		}
		if (IsAssetLoaded(handle))
		{
			asset = m_LoadedAssets.at(handle);
		} 
		else
		{
			const AssetMetadata& metadata = GetAssetMetadata(handle);
			AsyncLoadRequest* request = new AsyncLoadRequest();
			request->Handle = handle;
			request->FilePath = metadata.FilePath;
			request->State = AssetState::NotLoaded;
			request->Priority = LoadPriority::Normal;

			m_LoadingAssets[handle] = request;
			
		}
		return asset;
	}

}