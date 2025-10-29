#pragma once

#include "Panels/SceneHierarchyPanel.h"

namespace Gravix 
{

	class InspectorPanel 
	{
	public:
		InspectorPanel(SceneHierarchyPanel sceneHierarchyPanel);
		~InspectorPanel() = default;

		void OnImGuiRender();

	private:
		void DrawComponents(Entity entity);
	private:
		SceneHierarchyPanel m_SceneHierarchyPanel;
	};

}