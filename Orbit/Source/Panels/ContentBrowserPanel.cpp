#include "ContentBrowserPanel.h"

#include <filesystem>
#include <imgui.h>

namespace Gravix 
{

	ContentBrowserPanel::ContentBrowserPanel()
		: m_AssetDirectory(Project::GetAssetDirectory())
	{
		m_TreeNodes.push_back(TreeNode(".", 0));
		m_CurrentDirectory = m_AssetDirectory;

		m_DirectoryIcon = Texture2D::Create("EditorAssets/Icons/ContentBrowser/DirectoryIcon.png");
		m_FileIcon = Texture2D::Create("EditorAssets/Icons/ContentBrowser/FileIcon.png");

		RefreshAssetTree();
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");

		const char* label = m_Mode == Mode::FileSystem ? "File" : "Asset";
		if (ImGui::Button(label)) 
		{
			m_Mode = m_Mode == Mode::Asset ? Mode::FileSystem : Mode::Asset;
		}

		// Unity-style navigation bar with back button
		if(m_CurrentDirectory != m_AssetDirectory)
		{
			ImGui::SameLine();
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

		if (m_Mode == Mode::Asset) 
		{
			TreeNode* node = &m_TreeNodes[0];

			auto currentDir = std::filesystem::relative(m_CurrentDirectory, m_AssetDirectory);
			for (const auto& p : currentDir) 
			{
				if (node->Path == currentDir)
					break;

				if (node->Children.find(p) != node->Children.end()) 
				{
					node = &m_TreeNodes[node->Children[p]];
					continue;
				}
			}

			for (const auto& [item, treeNodeIndex] : node->Children)
			{
				bool isDirectory = std::filesystem::is_directory(m_AssetDirectory / item);
				std::string itemStr = item.generic_string();

				ImGui::PushID(itemStr.c_str());

				Ref<Texture2D> icon = isDirectory ? m_DirectoryIcon : m_FileIcon;

				// Unity-style asset button with hover effect
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.267f, 0.529f, 0.808f, 0.2f));  // Unity blue tint
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.267f, 0.529f, 0.808f, 0.4f));

				ImGui::ImageButton(itemStr.c_str(), (ImTextureID)icon->GetImGuiAttachment(), { thumbnailSize, thumbnailSize });

				if (!isDirectory)
				{
					if (ImGui::BeginPopupContextItem())
					{
						if (ImGui::MenuItem("Delete"))
						{
							
						}

						ImGui::EndPopup();
					}
				}

				if (ImGui::BeginDragDropSource()) 
				{
					AssetHandle handle = m_TreeNodes[treeNodeIndex].Handle;
					ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", &handle, sizeof(AssetHandle));
					ImGui::EndDragDropSource();
				}

				ImGui::PopStyleColor(3);
				if (ImGui::IsItemHovered())
				{
					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && isDirectory)
						m_CurrentDirectory /= item.filename();
				}

				// Center-aligned text for file names
				ImVec2 textSize = ImGui::CalcTextSize(itemStr.c_str());
				float textOffset = (thumbnailSize - textSize.x) * 0.5f;
				if (textOffset > 0.0f)
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textOffset);

				ImGui::TextWrapped("%s", itemStr.c_str());

				ImGui::PopID();
				ImGui::NextColumn();
			}
		}
		else 
		{
			for (auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
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

				ImGui::ImageButton(path.string().c_str(), (ImTextureID)icon->GetImGuiAttachment(), { thumbnailSize, thumbnailSize });

				if (!entry.is_directory())
				{
					if (ImGui::BeginPopupContextItem())
					{
						if (ImGui::MenuItem("Import"))
						{
							Project::GetActive()->GetEditorAssetManager()->ImportAsset(relativePath);
							RefreshAssetTree();
						}

						ImGui::EndPopup();
					}
				}

				ImGui::PopStyleColor(3);

				if (ImGui::IsItemHovered())
				{
					if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && entry.is_directory())
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
		}

		ImGui::Columns(1);
		ImGui::End();
	}

	void ContentBrowserPanel::RefreshAssetTree()
	{
		const auto& assetRegistry = Project::GetActive()->GetEditorAssetManager()->GetAssetRegistry();
		for(const auto& [handle, metadata] : assetRegistry)
		{
			uint32_t currentNodeIndex = 0;
			for (const auto& p : metadata.FilePath) 
			{
				auto it = m_TreeNodes[currentNodeIndex].Children.find(p.generic_string());
				if(it != m_TreeNodes[currentNodeIndex].Children.end())
				{
					currentNodeIndex = it->second;
				}
				else 
				{
					TreeNode newNode(p, handle);
					newNode.Parent = currentNodeIndex;
					m_TreeNodes.push_back(newNode);

					m_TreeNodes[currentNodeIndex].Children[p] = m_TreeNodes.size() - 1;
					currentNodeIndex = m_TreeNodes.size() - 1;
				}
			}
		}
	}

}