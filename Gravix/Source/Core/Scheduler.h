#pragma once

#include "Asset/Asset.h"
#include "Asset/AssetMetadata.h"

#include "Asset/AssetManager.h"

#include "Asset/Importers/TextureImporter.h"
#include "Asset/Importers/SceneImporter.h"

#include "Core/Application.h"
#include "Core/Buffer.h"

#include <TaskScheduler.h>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <vector>
#include <unordered_set>
#include <variant>

namespace Gravix
{

	enum class LoadPriority
	{
		Low = enki::TASK_PRIORITY_LOW,
		Normal = enki::TASK_PRIORITY_MED,
		High = enki::TASK_PRIORITY_HIGH
	};

	struct AsyncLoadRequest
	{
		AssetHandle Handle = 0;
		std::filesystem::path FilePath;
		LoadPriority Priority = LoadPriority::Normal;
		AssetState State = AssetState::NotLoaded;
		enki::TaskSet* LoadTaskSet = nullptr;

		struct TextureData 
		{
			Buffer Data;
			uint32_t Width, Height, Channels;
		};

		struct SceneData
		{
			YAML::Node SceneNode;
			std::vector<AssetHandle> Dependencies;
		};

		std::variant<std::monostate, TextureData, SceneData> CPUData;
	};

	struct AsyncLoadTask : public enki::ITaskSet
	{
		std::vector<AsyncLoadRequest*> LoadRequests;

		AsyncLoadTask(size_t count)
		{
			m_SetSize = static_cast<uint32_t>(count);
		}

		void ExecuteRange(enki::TaskSetPartition range_, uint32_t threadnum_) override
		{
			for (uint32_t i = range_.start; i < range_.end; ++i)
			{
				AsyncLoadRequest* request = LoadRequests[i];
				LoadAsset(request);
				AssetManager::PushToCompletionQueue(request);
				request->State = AssetState::ReadyForGPU;
			}
		}

		void LoadAsset(AsyncLoadRequest* request)
		{
			if (!Application::Get().IsRuntime()) 
			{
				if (Project::GetActive()->GetEditorAssetManager()->IsAssetHandleValid(request->Handle))
				{
					AssetMetadata metadata = Project::GetActive()->GetEditorAssetManager()->GetAssetMetadata(request->Handle);
					SetCPUDataEditor(request, metadata);
				}
				else
				{
					AssetHandle handle = Project::GetActive()->GetEditorAssetManager()->ImportAsset(request->FilePath);
					AssetMetadata metadata = Project::GetActive()->GetEditorAssetManager()->GetAssetMetadata(handle);
					SetCPUDataEditor(request, metadata);
					request->Handle = handle;
				}
			}
		}

		void SetCPUDataEditor(AsyncLoadRequest* request, AssetMetadata metadata)
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