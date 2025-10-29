#pragma once

#include "Panels/SceneHierarchyPanel.h"

namespace Gravix 
{

	class InspectorPanel 
	{
	public:
		InspectorPanel() = default;
		InspectorPanel(SceneHierarchyPanel sceneHierarchyPanel);
		~InspectorPanel() = default;

		void SetSceneHierarchyPanel(SceneHierarchyPanel sceneHierarchyPanel) { m_SceneHierarchyPanel = sceneHierarchyPanel; }

		void OnImGuiRender();
	private:
		void DrawComponents(Entity entity);
	private:
		SceneHierarchyPanel m_SceneHierarchyPanel;
	};

}