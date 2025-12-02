#pragma once

#include "Asset/Asset.h"
#include "Asset/AssetMetadata.h"
#include "Asset/AsyncLoadRequest.h"

#include "Asset/AssetManager.h"

#include "Asset/AssetImporter.h"
#ifdef GRAVIX_EDITOR_BUILD
#include "Asset/Importers/TextureImporter.h"
#include "Asset/Importers/SceneImporter.h"
#endif

#include "Project/Project.h"
#include "Core/Application.h"

#include <TaskScheduler.h>
#ifdef GRAVIX_EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif
#include <filesystem>
#include <vector>
#include <unordered_set>

namespace Gravix
{

	struct AsyncLoadTask : public enki::ITaskSet
	{
		std::vector<Ref<AsyncLoadRequest>> LoadRequests;

		AsyncLoadTask(size_t count)
		{
			m_SetSize = static_cast<uint32_t>(count);
		}

		void ExecuteRange(enki::TaskSetPartition range_, uint32_t threadnum_) override
		{
			for (uint32_t i = range_.start; i < range_.end; ++i)
			{
				Ref<AsyncLoadRequest> request = LoadRequests[i];
				LoadAsset(request);
				AssetManager::PushToCompletionQueue(request);
				request->State = AssetState::ReadyForGPU;
			}
		}

		void LoadAsset(Ref<AsyncLoadRequest> request)
		{
#ifdef GRAVIX_EDITOR_BUILD
			if (!Application::Get().IsRuntime())
			{
				if (Project::GetActive()->GetEditorAssetManager()->IsAssetHandleValid(request->Handle))
				{
					AssetMetadata metadata = Project::GetActive()->GetEditorAssetManager()->GetAssetMetadata(request->Handle);
					SetCPUDataEditor(request, metadata);
				}
			}
#endif
		}

#ifdef GRAVIX_EDITOR_BUILD
		void SetCPUDataEditor(Ref<AsyncLoadRequest> request, AssetMetadata metadata)
		{
			if (metadata.Type == AssetType::Texture2D)
			{
				int width, height, channels;
				Buffer data = TextureImporter::LoadTexture2DToBuffer(Project::GetAssetDirectory() / request->FilePath, &width, &height, &channels);
				AsyncLoadRequest::TextureData textureData = {
					data,
					(uint32_t)width,
					(uint32_t)height,
					(uint32_t)channels
				};

				request->CPUData = textureData;
			}
			else if (metadata.Type == AssetType::Scene)
			{
				std::vector<AssetHandle> dependencies;
				YAML::Node sceneNode = SceneImporter::LoadSceneToYAML(Project::GetAssetDirectory() / request->FilePath, &dependencies);
				AsyncLoadRequest::SceneData sceneData = {
					sceneNode,
					dependencies
				};
				request->CPUData = sceneData;
			}
		}
#endif
	};

	struct RunPinnedTaskLoop : public enki::IPinnedTask
	{
		void Execute() override
		{
			while (TaskSch->GetIsRunning() && ExecuteTasks)
			{
				TaskSch->WaitForNewPinnedTasks(); // this thread will 'sleep' until there are new pinned tasks
				TaskSch->RunPinnedTasks();
			}
		}

		enki::TaskScheduler* TaskSch;
		bool ExecuteTasks = true;
	};

	class Scheduler
	{
	public:
		Scheduler() = default;
		~Scheduler() = default;

		// Initialize with specified number of threads (default to number of hardware threads)
		void Init(uint32_t threadCount = std::thread::hardware_concurrency());

		//void CreateRunPinnedTaskLoop(const RunPinnedTaskLoop& runTask) { m_TaskScheduler.AddPinnedTask(runTask); }

		enki::TaskScheduler& GetTaskScheduler() { return m_TaskScheduler; }
	private:
		enki::TaskScheduler m_TaskScheduler;
	};
}