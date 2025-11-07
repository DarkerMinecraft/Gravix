#include "ContentBrowserPanel.h"

#include <filesystem>
#include <imgui.h>

namespace Gravix 
{

	ContentBroswerPanel::ContentBroswerPanel()
		: m_AssetDirectory(Application::Get().GetProject().GetAssetsDirectory())
	{
		m_CurrentDirectory = m_AssetDirectory;

		m_DirectoryIcon = Texture2D::Create("EditorAssets/Icons/ContentBrowser/DirectoryIcon.png");
		m_FileIcon = Texture2D::Create("EditorAssets/Icons/ContentBrowser/FileIcon.png");
	}

	void ContentBroswerPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");
		
		if(m_CurrentDirectory != m_AssetDirectory)
		{
			if (ImGui::Button("<-"))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
		}

		float padding = 16.0f;
		float thumbnailSize = 64.0f;
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
			
			Ref<Texture2D> icon = entry.is_directory() ? m_DirectoryIcon : m_FileIcon;
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::ImageButton(path.string().c_str(), (ImTextureID)icon->GetImGuiAttachment(), {thumbnailSize, thumbnailSize});
			if (ImGui::BeginDragDropSource()) 
			{
				const wchar_t* payloadPath = relativePath.c_str();
				ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", payloadPath, (wcslen(payloadPath) + 1) * sizeof(wchar_t), ImGuiCond_Once);
				ImGui::EndDragDropSource();
			}
			ImGui::PopStyleColor();

			if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if(entry.is_directory())
					m_CurrentDirectory /= path.filename();
			}
			ImGui::TextWrapped(fileNameString.c_str());
			ImGui::NextColumn();
		}

		ImGui::Columns(1);
		ImGui::End();
	}

}