#include "ContentBrowserPanel.h"
#include "AppLayer.h"

#include "Serialization/Scene/SceneSerializer.h"
#include "Scene/Scene.h"

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
				CreateNewScene();
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
				// Create new folder in current directory
				std::filesystem::path newFolderPath = m_CurrentDirectory / "New Folder";
				int counter = 1;
				while (std::filesystem::exists(newFolderPath))
				{
					newFolderPath = m_CurrentDirectory / ("New Folder " + std::to_string(counter++));
				}

				try
				{
					std::filesystem::create_directory(newFolderPath);
					GX_CORE_INFO("Created folder: {0}", newFolderPath.filename().string());
					RefreshAssetTree();
				}
				catch (const std::filesystem::filesystem_error& e)
				{
					GX_CORE_ERROR("Failed to create folder: {0}", e.what());
				}

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
			std::filesystem::path fullPath = m_CurrentDirectory / item.filename();
			bool isDirectory = std::filesystem::is_directory(fullPath);
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

			// Right-click context menu
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Rename"))
				{
					m_IsRenaming = true;
					m_RenamingPath = item;
					// Get name without extension for files
					std::string displayName = isDirectory ? item.filename().string() : item.stem().string();
					strncpy(m_RenameBuffer, displayName.c_str(), sizeof(m_RenameBuffer) - 1);
				}

				if (ImGui::MenuItem("Delete"))
				{
					// TODO: Implement delete functionality
				}

				ImGui::EndPopup();
			}

			// Center-aligned text for file names (hide extension for files)
			std::string displayName = isDirectory ? itemStr : std::filesystem::path(itemStr).stem().string();
			ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
			float textOffset = (thumbnailSize - textSize.x) * 0.5f;
			if (textOffset > 0.0f)
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textOffset);

			ImGui::TextWrapped("%s", displayName.c_str());

			ImGui::PopID();
			ImGui::NextColumn();
		}

		ImGui::Columns(1);

		// Rename dialog
		if (m_IsRenaming)
		{
			ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
			ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_Always);

			if (!ImGui::IsPopupOpen("Rename Asset"))
			{
				ImGui::OpenPopup("Rename Asset");
			}

			if (ImGui::BeginPopupModal("Rename Asset", &m_IsRenaming, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
			{
				ImGui::Text("Enter new name:");
				ImGui::Spacing();

				ImGui::SetNextItemWidth(-1);
				if (ImGui::InputText("##AssetName", m_RenameBuffer, sizeof(m_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
				{
					if (strlen(m_RenameBuffer) > 0)
					{
						RenameAsset(m_RenamingPath, m_RenameBuffer);
						m_IsRenaming = false;
						ImGui::CloseCurrentPopup();
					}
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				float buttonWidth = 100.0f;
				float spacing = 10.0f;
				float totalWidth = buttonWidth * 2 + spacing;
				ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalWidth) * 0.5f);

				if (ImGui::Button("Rename", ImVec2(buttonWidth, 0)))
				{
					if (strlen(m_RenameBuffer) > 0)
					{
						RenameAsset(m_RenamingPath, m_RenameBuffer);
						m_IsRenaming = false;
						ImGui::CloseCurrentPopup();
					}
				}

				ImGui::SameLine(0, spacing);

				if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
				{
					m_IsRenaming = false;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}

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
					Ref<EditorAssetManager> assetManager = Project::GetActive()->GetEditorAssetManager();
					assetManager->ImportAsset(relativePath);
					assetManager->SerializeAssetRegistry();
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

		// First, add all files from the asset registry
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

		// Now scan for all directories (including empty ones) and add them to the tree
		try
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(m_AssetDirectory))
			{
				if (!entry.is_directory())
					continue;

				auto relativePath = std::filesystem::relative(entry.path(), m_AssetDirectory);
				uint32_t currentNodeIndex = 0;

				// Navigate/create the path in the tree
				for (const auto& p : relativePath)
				{
					auto it = m_TreeNodes[currentNodeIndex].Children.find(p.generic_string());
					if(it != m_TreeNodes[currentNodeIndex].Children.end())
					{
						currentNodeIndex = it->second;
					}
					else
					{
						TreeNode newNode(p, 0); // Directories don't have asset handles
						newNode.Parent = currentNodeIndex;
						m_TreeNodes.push_back(newNode);

						m_TreeNodes[currentNodeIndex].Children[p] = m_TreeNodes.size() - 1;
						currentNodeIndex = m_TreeNodes.size() - 1;
					}
				}
			}
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			GX_CORE_ERROR("Failed to scan directories: {0}", e.what());
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

			// Serialize registry after importing new assets
			assetManager->SerializeAssetRegistry();
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			GX_CORE_ERROR("Failed to scan asset directory: {0}", e.what());
		}
	}

	void ContentBrowserPanel::RenameAsset(const std::filesystem::path& oldPath, const std::string& newName)
	{
		// Build full paths using current directory (not asset directory)
		std::filesystem::path fullOldPath = m_CurrentDirectory / oldPath;
		std::filesystem::path extension = oldPath.extension();
		std::filesystem::path newFilename = newName + extension.string();
		std::filesystem::path fullNewPath = fullOldPath.parent_path() / newFilename;

		// Calculate relative path from asset directory for metadata lookup
		std::filesystem::path oldRelativePath = std::filesystem::relative(fullOldPath, m_AssetDirectory);

		try
		{
			// Rename the file on disk
			std::filesystem::rename(fullOldPath, fullNewPath);

			// Update asset metadata in the asset manager
			Ref<EditorAssetManager> assetManager = Project::GetActive()->GetEditorAssetManager();
			const auto& assetRegistry = assetManager->GetAssetRegistry();

			// Find the asset handle for the old path
			for (auto& [handle, metadata] : assetRegistry)
			{
				if (metadata.FilePath == oldRelativePath)
				{
					// Update the metadata file path
					auto& mutableMetadata = const_cast<AssetMetadata&>(metadata);
					mutableMetadata.FilePath = std::filesystem::relative(fullNewPath, m_AssetDirectory);

					// Serialize the updated asset registry
					assetManager->SerializeAssetRegistry();

					// Refresh the asset tree
					m_TreeNodes.clear();
					m_TreeNodes.push_back(TreeNode(".", 0));
					RefreshAssetTree();
					ScanAndImportAssets();

					// Update window title if this is the active scene
					if (m_AppLayer && metadata.Type == AssetType::Scene && handle == m_AppLayer->GetActiveSceneHandle())
					{
						m_AppLayer->UpdateWindowTitle();
					}

					GX_CORE_INFO("Renamed asset: {0} -> {1}", oldRelativePath.string(), mutableMetadata.FilePath.string());
					break;
				}
			}
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			GX_CORE_ERROR("Failed to rename asset: {0}", e.what());
		}
	}

	void ContentBrowserPanel::CreateNewScene()
	{
		// Generate a unique scene file name
		std::filesystem::path newScenePath = m_CurrentDirectory / "NewScene.orbscene";
		int counter = 1;
		while (std::filesystem::exists(newScenePath))
		{
			newScenePath = m_CurrentDirectory / ("NewScene" + std::to_string(counter++) + ".orbscene");
		}

		try
		{
			// Create a new empty scene
			Ref<Scene> newScene = CreateRef<Scene>();

			// Serialize the scene to a file
			SceneSerializer serializer(newScene);
			serializer.Serialize(newScenePath);

			// Import the scene into the asset manager
			auto relativePath = std::filesystem::relative(newScenePath, m_AssetDirectory);
			Ref<EditorAssetManager> assetManager = Project::GetActive()->GetEditorAssetManager();
			assetManager->ImportAsset(relativePath);
			assetManager->SerializeAssetRegistry();

			// Refresh the asset tree to show the new scene
			RefreshAssetTree();

			GX_CORE_INFO("Created new scene: {0}", newScenePath.filename().string());
		}
		catch (const std::exception& e)
		{
			GX_CORE_ERROR("Failed to create new scene: {0}", e.what());
		}
	}

}