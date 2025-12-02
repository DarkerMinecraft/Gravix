#pragma once

#include "Asset/Asset.h"
#include "Core/Buffer.h"

#include "../../../ThirdParties/enkiTS/src/TaskScheduler.h"

#ifdef GRAVIX_EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

#include <filesystem>
#include <vector>
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
#ifdef GRAVIX_EDITOR_BUILD
			YAML::Node SceneNode;
#else GRAVIX_RUNTIME_BUILD
			Buffer SceneNode;
#endif 
			std::vector<AssetHandle> Dependencies;
		};

		std::variant<std::monostate, TextureData, SceneData> CPUData;
	};
}
