#pragma once

#include "Panels/SceneHierarchyPanel.h"

namespace Gravix
{

	class AppLayer;

	class InspectorPanel
	{
	public:
		InspectorPanel() = default;
		InspectorPanel(SceneHierarchyPanel* sceneHierarchyPanel);
		~InspectorPanel() = default;

		void SetSceneHierarchyPanel(SceneHierarchyPanel* sceneHierarchyPanel) { m_SceneHierarchyPanel = sceneHierarchyPanel; }
		void SetAppLayer(AppLayer* appLayer) { m_AppLayer = appLayer; }

		void OnImGuiRender();
	private:
		void DrawComponents(Entity entity);
		void DrawAddComponents(Entity entity);
	private:
		SceneHierarchyPanel* m_SceneHierarchyPanel;
		AppLayer* m_AppLayer = nullptr;
	};

}