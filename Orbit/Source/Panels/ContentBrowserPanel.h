#pragma once

#include "Core/Application.h"

#include "Renderer/Generic/Types/Texture.h"

namespace Gravix 
{

	class ContentBroswerPanel 
	{
	public:
		ContentBroswerPanel();
		~ContentBroswerPanel() = default;

		void OnImGuiRender();
	private:
		std::filesystem::path m_AssetDirectory;
		std::filesystem::path m_CurrentDirectory;

		Ref<Texture2D> m_DirectoryIcon;
		Ref<Texture2D> m_FileIcon;
	};

}