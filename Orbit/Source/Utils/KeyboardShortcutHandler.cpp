#include "KeyboardShortcutHandler.h"

#include "ProjectManager.h"
#include "SceneManager.h"
#include "Panels/SceneHierarchyPanel.h"

#include "Core/Input.h"

#include <imgui.h>

namespace Gravix
{

	bool KeyboardShortcutHandler::HandleKeyPress(KeyPressedEvent& e)
	{
		// Don't process shortcuts if ImGui wants keyboard input
		ImGuiIO& io = ImGui::GetIO();
		if (io.WantCaptureKeyboard && (io.WantTextInput || ImGui::IsAnyItemActive()))
			return false;

		bool ctrlDown = Input::IsKeyDown(Key::LeftControl) || Input::IsKeyDown(Key::RightControl);
		bool shiftDown = Input::IsKeyDown(Key::LeftShift) || Input::IsKeyDown(Key::RightShift);

		switch (e.GetKeyCode())
		{
		case Key::N:
			if (ctrlDown)
				return HandleNewProject();
			break;

		case Key::O:
			if (ctrlDown)
				return HandleOpenProject();
			break;

		case Key::S:
			if (ctrlDown)
				return HandleSave(shiftDown);
			break;

		case Key::D:
			if (ctrlDown)
				return HandleDuplicate();
			break;
		}

		return false;
	}

	bool KeyboardShortcutHandler::HandleNewProject()
	{
		if (!m_ProjectManager)
			return false;

		if (m_ProjectManager->CreateNewProject())
		{
			if (m_OnProjectCreated)
				m_OnProjectCreated();
			return true;
		}

		return false;
	}

	bool KeyboardShortcutHandler::HandleOpenProject()
	{
		if (!m_ProjectManager)
			return false;

		if (m_ProjectManager->OpenProject())
		{
			if (m_OnProjectOpened)
				m_OnProjectOpened();
			return true;
		}

		return false;
	}

	bool KeyboardShortcutHandler::HandleSave(bool saveAs)
	{
		if (!m_SceneManager && !m_ProjectManager)
			return false;

		if (saveAs)
		{
			if (m_ProjectManager)
				m_ProjectManager->SaveActiveProjectAs();
		}
		else
		{
			if (m_SceneManager)
				m_SceneManager->SaveActiveScene();
		}

		return true;
	}

	bool KeyboardShortcutHandler::HandleDuplicate()
	{
		if (!m_SceneManager || !m_SceneHierarchyPanel)
			return false;

		if (m_SceneManager->GetSceneState() != SceneState::Edit)
			return false;

		Entity entity = m_SceneHierarchyPanel->GetSelectedEntity();
		if (entity)
		{
			m_SceneManager->GetActiveScene()->DuplicateEntity(entity);
			m_SceneManager->MarkSceneDirty();
			return true;
		}

		return false;
	}

}
