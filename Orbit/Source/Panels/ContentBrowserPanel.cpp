#include "ContentBrowserPanel.h"

#include <filesystem>
#include <imgui.h>

namespace Gravix 
{

	ContentBrowserPanel::ContentBrowserPanel()
	{
		m_TreeNodes.push_back(TreeNode(".", 0));

		m_DirectoryIcon = Texture2D::Create("EditorAssets/Icons/ContentBrowser/DirectoryIcon.png");
		m_FileIcon = Texture2D::Create("EditorAssets/Icons/ContentBrowser/FileIcon.png");

		m_AssetDirectory = Project::GetAssetDirectory();
		m_CurrentDirectory = m_AssetDirectory;

		// Auto-load all assets from filesystem
		ScanAndImportAssets();
		RefreshAssetTree();
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser");

		ImGuiIO& io = ImGui::GetIO();

		// Top toolbar with modern styling
		ImGui::BeginGroup();

		// Create button with dropdown menu
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.267f, 0.529f, 0.808f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.353f, 0.627f, 0.902f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.208f, 0.471f, 0.749f, 1.0f));
		ImGui::PushFont(io.Fonts->Fonts[1]); // Bold font
		if (ImGui::Button("+ Create", ImVec2(80.0f, 0.0f)))
		{
			ImGui::OpenPopup("CreateAssetPopup");
		}
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		// Create asset popup menu
		if (ImGui::BeginPopup("CreateAssetPopup"))
		{
			ImGui::PushFont(io.Fonts->Fonts[1]);
			ImGui::TextColored(ImVec4(0.267f, 0.529f, 0.808f, 1.0f), "Create New Asset");
			ImGui::PopFont();
			ImGui::Separator();

			if (ImGui::MenuItem("Scene"))
			{
				// TODO: Create new scene file
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("Material"))
			{
				// TODO: Create new material
				ImGui::CloseCurrentPopup();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Folder"))
			{
				// TODO: Create new folder
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::MenuItem("C++ Script"))
			{
				// TODO: Create new C++ script
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
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
		}

		ImGui::EndGroup();

		// Display current path (Unity-style breadcrumb) with bold font
		ImGui::Spacing();
		std::string displayPath = std::filesystem::relative(m_CurrentDirectory, m_AssetDirectory).string();
		if (displayPath.empty() || displayPath == ".")
			displayPath = "Assets";

		ImGui::PushFont(io.Fonts->Fonts[1]);
		ImGui::TextColored(ImVec4(0.863f, 0.863f, 0.863f, 1.0f), "Path:");
		ImGui::PopFont();
		ImGui::SameLine();
		ImGui::TextDisabled("%s", displayPath.c_str());

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

		ImGui::Columns(1);

		ImGui::End();
	}

	void ContentBrowserPanel::OnFileDrop(const std::vector<std::string>& paths)
	{
		for (const auto& sourcePath : paths)
		{
			std::filesystem::path fsPath(sourcePath);

			if (std::filesystem::exists(fsPath) && std::filesystem::is_regular_file(fsPath))
			{
				// Copy file to current directory in Content Browser
				std::filesystem::path destinationPath = m_CurrentDirectory / fsPath.filename();

				try
				{
					std::filesystem::copy_file(fsPath, destinationPath, std::filesystem::copy_options::overwrite_existing);

					// Import the newly copied asset
					auto relativePath = std::filesystem::relative(destinationPath, m_AssetDirectory);
					Project::GetActive()->GetEditorAssetManager()->ImportAsset(relativePath);
					RefreshAssetTree();

					GX_CORE_INFO("Imported external file: {0}", fsPath.filename().string());
				}
				catch (const std::exception& e)
				{
					GX_CORE_ERROR("Failed to copy file: {0}", e.what());
				}
			}
		}
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

	void ContentBrowserPanel::ScanAndImportAssets()
	{
		// Check if asset directory exists
		if (!std::filesystem::exists(m_AssetDirectory))
		{
			GX_CORE_WARN("Asset directory does not exist: {0}", m_AssetDirectory.string());
			return;
		}

		Ref<EditorAssetManager> assetManager = Project::GetActive()->GetEditorAssetManager();
		const auto& assetRegistry = assetManager->GetAssetRegistry();

		// Build a set of already registered file paths for efficient lookup
		std::set<std::filesystem::path> registeredPaths;
		for (const auto& [handle, metadata] : assetRegistry)
		{
			registeredPaths.insert(metadata.FilePath);
		}

		// Recursively scan all files in the asset directory
		try
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(m_AssetDirectory))
			{
				if (!entry.is_regular_file())
					continue;

				auto relativePath = std::filesystem::relative(entry.path(), m_AssetDirectory);

				// Check if this file is already in the registry
				if (registeredPaths.find(relativePath) == registeredPaths.end())
				{
					// Not in registry, import it asynchronously
					assetManager->ImportAsset(relativePath);
				}
			}
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			GX_CORE_ERROR("Failed to scan asset directory: {0}", e.what());
		}
	}

}