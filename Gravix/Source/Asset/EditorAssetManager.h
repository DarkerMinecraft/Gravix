#pragma once

#include "AssetManagerBase.h"
#include "AssetMetadata.h"

#include <mutex>
#include <queue>

namespace Gravix 
{

	using AssetRegistry = std::map<AssetHandle, AssetMetadata>;

	class EditorAssetManager : public AssetManagerBase
	{
	public:
		virtual Ref<Asset> GetAsset(AssetHandle handle) override;

		virtual bool IsAssetLoaded(AssetHandle handle) const override;
		virtual bool IsAssetHandleValid(AssetHandle handle) const override;
		virtual AssetType GetAssetType(AssetHandle handle) const override;

		virtual void PushToCompletionQueue(Ref<AsyncLoadRequest> request) override;
		virtual void ProcessAsyncLoads() override;

		void ImportAsset(const std::filesystem::path& filePath);

		const AssetMetadata& GetAssetMetadata(AssetHandle handle) const;
		const AssetRegistry& GetAssetRegistry() const { return m_AssetRegistry; }

		const std::filesystem::path& GetAssetFilePath(AssetHandle handle) const;

		void ClearLoadedAssets();

		void SerializeAssetRegistry();
		void DeserializeAssetRegistry();
	private:
		AssetRegistry m_AssetRegistry;
		AssetMap m_LoadedAssets;

		std::unordered_map<AssetHandle, Ref<AsyncLoadRequest>> m_LoadingAssets;
		std::queue<Ref<AsyncLoadRequest>> m_CompletionQueue;
		std::mutex m_CompletionQueueMutex;

		// Reused vector to avoid allocations in ProcessAsyncLoads
		std::vector<Ref<AsyncLoadRequest>> m_CompletedRequestsCache;
	};
}