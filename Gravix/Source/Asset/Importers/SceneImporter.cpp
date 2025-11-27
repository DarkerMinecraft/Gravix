#include "pch.h"
#include "SceneImporter.h"

#include "Scene/Scene.h"
#include "Serialization/Scene/SceneSerializer.h"
#include "Project/Project.h"
#include "Physics/PhysicsWorld.h"

namespace Gravix 
{

	Ref<Asset> SceneImporter::ImportScene(AssetHandle handle, const AssetMetadata& metadata)
	{
		Ref<Scene> scene = CreateRef<Scene>();
		SceneSerializer serializer(scene);
		serializer.Deserialize(Project::GetAssetDirectory() / metadata.FilePath);

		return scene;
	}

	YAML::Node SceneImporter::LoadSceneToYAML(const std::filesystem::path& path, std::vector<AssetHandle>* outDependencies /*= nullptr*/)
	{
		Ref<Scene> scene = CreateRef<Scene>();
		SceneSerializer serializer(scene);
		YAML::Node node; 
		serializer.Deserialize(path, &node);

		scene->ExtractSceneDependencies(outDependencies);
		return node;
	}

}