#include "pch.h"
#include "SceneImporter.h"

#include "Scene/Scene.h"
#include "Serialization/Scene/SceneSerializer.h"
#include "Project/Project.h"

namespace Gravix 
{

	Ref<Asset> SceneImporter::ImportScene(AssetHandle handle, const AssetMetadata& metadata)
	{
		Ref<Scene> scene = CreateRef<Scene>();
		SceneSerializer serializer(scene);
		serializer.Deserialize(Project::GetAssetDirectory() / metadata.FilePath);

		return scene;
	}

}