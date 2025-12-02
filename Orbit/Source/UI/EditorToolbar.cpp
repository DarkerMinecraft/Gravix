#include "EditorToolbar.h"
#include "SceneManager.h"

#include "Asset/Importers/TextureImporter.h"

#include <imgui.h>

namespace Gravix
{

	EditorToolbar::EditorToolbar()
	{
		m_IconPlay = TextureImporter::LoadTexture2D("EditorAssets/Icons/PlayButton.png");
		m_IconStop = TextureImporter::LoadTexture2D("EditorAssets/Icons/StopButton.png");
	}

	void EditorToolbar::OnImGuiRender()
	{
		if (!m_SceneManager)
			return;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

		auto& colors = ImGui::GetStyle().Colors;
		auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
		auto& buttonActive = colors[ImGuiCol_ButtonActive];
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

		ImGui::Begin("##toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		float windowHeight = ImGui::GetWindowHeight();
		float buttonSize = windowHeight * 0.8f;

		Ref<Texture2D> icon = (m_SceneManager->GetSceneState() == SceneState::Edit) ? m_IconPlay : m_IconStop;
		ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (buttonSize * 0.5f));

		if (ImGui::ImageButton("SceneState", (ImTextureID)icon->GetImGuiAttachment(), ImVec2{ buttonSize, buttonSize }))
		{
			if (m_SceneManager->GetSceneState() == SceneState::Edit)
				m_SceneManager->Play();
			else if (m_SceneManager->GetSceneState() == SceneState::Play)
				m_SceneManager->Stop();
		}

		ImGui::PopStyleVar(4);
		ImGui::PopStyleColor(3);
		ImGui::End();
	}

}
