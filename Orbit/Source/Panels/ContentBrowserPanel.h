#pragma once

#include "Core/Application.h"

#include "Renderer/Generic/Types/Texture.h"
#include "Asset/Asset.h"

#include <map>
#include <vector>
#include <set>

namespace Gravix 
{

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();
		~ContentBrowserPanel() = default;

		void OnImGuiRender();
		void OnFileDrop(const std::vector<std::string>& paths);
	private:
		void RefreshAssetTree();
		void ScanAndImportAssets();
		void RenameAsset(const std::filesystem::path& oldPath, const std::string& newName);
	private:
		std::filesystem::path m_AssetDirectory;
		std::filesystem::path m_CurrentDirectory;

		Ref<Texture2D> m_DirectoryIcon;
		Ref<Texture2D> m_FileIcon;

		struct TreeNode
		{
			std::filesystem::path Path;
			AssetHandle Handle = 0;

			uint32_t Parent = (uint32_t)-1;
			std::map<std::filesystem::path, uint32_t> Children;

			TreeNode(const std::filesystem::path& path, AssetHandle handle)
				: Path(path), Handle(handle) {}
		};

		std::vector<TreeNode> m_TreeNodes;

		// Renaming state
		bool m_IsRenaming = false;
		std::filesystem::path m_RenamingPath;
		char m_RenameBuffer[256] = "";
	};

}