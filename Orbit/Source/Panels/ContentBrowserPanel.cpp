#include "ContentBrowserPanel.h"

#include <filesystem>
#include <imgui.h>

namespace Gravix 
{

	ContentBrowserPanel::ContentBrowserPanel()
		: m_AssetDirectory(Project::GetAssetDirectory())
	{
		m_CurrentDirectory = m_AssetDirectory;

		m_DirectoryIcon = Texture2D::Create("EditorAssets/Icons/ContentBrowser/DirectoryIcon.png");
		m_FileIcon = Texture2D::Create("EditorAssets/Icons/ContentBrowser/FileIcon.png");
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");

		// Unity-style navigation bar with back button
		if(m_CurrentDirectory != m_AssetDirectory)
		{
			// Styled back button
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.267f, 0.267f, 0.267f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.349f, 0.349f, 0.349f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.443f, 0.443f, 0.443f, 1.0f));

			if (ImGui::Button("< Back"))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}

			ImGui::PopStyleColor(3);

			ImGui::SameLine();
		}

		// Display current path (Unity-style breadcrumb)
		std::string displayPath = std::filesystem::relative(m_CurrentDirectory, m_AssetDirectory).string();
		if (displayPath.empty() || displayPath == ".")
			displayPath = "Assets";
		ImGui::TextDisabled("Path: %s", displayPath.c_str());

		ImGui::Separator();
		ImGui::Spacing();

		// Unity-style asset grid
		float padding = 16.0f;
		float thumbnailSize = 80.0f;  // Slightly larger for better visibility
		float cellSize = thumbnailSize + padding;

		float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		ImGui::Columns(columnCount, 0, false);

		for(auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
		{
			const auto& path = entry.path();
			auto relativePath = std::filesystem::relative(path, m_AssetDirectory);
			std::string fileNameString = relativePath.filename().string();

			ImGui::PushID(fileNameString.c_str());

			Ref<Texture2D> icon = entry.is_directory() ? m_DirectoryIcon : m_FileIcon;

			// Unity-style asset button with hover effect
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.267f, 0.529f, 0.808f, 0.2f));  // Unity blue tint
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.267f, 0.529f, 0.808f, 0.4f));

			ImGui::ImageButton(path.string().c_str(), (ImTextureID)icon->GetImGuiAttachment(), {thumbnailSize, thumbnailSize});

			if (ImGui::BeginDragDropSource())
			{
				const wchar_t* payloadPath = relativePath.c_str();
				ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", payloadPath, (wcslen(payloadPath) + 1) * sizeof(wchar_t), ImGuiCond_Once);
				ImGui::EndDragDropSource();
			}

			ImGui::PopStyleColor(3);

			if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if(entry.is_directory())
					m_CurrentDirectory /= path.filename();
			}

			// Center-aligned text for file names
			ImVec2 textSize = ImGui::CalcTextSize(fileNameString.c_str());
			float textOffset = (thumbnailSize - textSize.x) * 0.5f;
			if (textOffset > 0.0f)
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textOffset);

			ImGui::TextWrapped("%s", fileNameString.c_str());

			ImGui::PopID();
			ImGui::NextColumn();
		}

		ImGui::Columns(1);
		ImGui::End();
	}

}