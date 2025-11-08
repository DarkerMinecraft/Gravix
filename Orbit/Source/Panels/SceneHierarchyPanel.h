#pragma once

#include "Scene/Scene.h"
#include "Scene/Entity.h"

namespace Gravix
{

	class AppLayer;

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene);
		~SceneHierarchyPanel() = default;

		void SetContext(const Ref<Scene>& scene);
		Ref<Scene> GetContext() const { return m_Context; }

		void SetAppLayer(AppLayer* appLayer) { m_AppLayer = appLayer; }

		void OnImGuiRender();

		const Entity GetSelectedEntity() const { return m_SelectedEntity; }
		void SetSelectedEntity(Entity entity) { m_SelectedEntity = entity; }

		void SetNoneSelected() { m_SelectedEntity = Entity{ entt::null, m_Context.get() }; }
	private:
		void DrawEntityNode(Entity entity);
	private:
		Ref<Scene> m_Context;
		Entity m_SelectedEntity;
		AppLayer* m_AppLayer = nullptr;
	};

}