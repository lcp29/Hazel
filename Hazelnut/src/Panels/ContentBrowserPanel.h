#pragma once

#include <filesystem>

namespace Hazel {

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel(void* directoryIcon, void* fileIcon);

		void OnImGuiRender();
	private:
		std::filesystem::path m_BaseDirectory;
		std::filesystem::path m_CurrentDirectory;

		void* m_DirectoryIcon = nullptr;
		void* m_FileIcon = nullptr;
	};

}
