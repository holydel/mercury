module;
#include <imgui.h>

export module EditorMainWindow;

import ShellOS;
import EditorState;
import ImguiState;

import AssetsBrowserWindow;
import SceneHierarchyWindow;
import PropertyWindow;

export class EditorMainWindow
{
	AssetsBrowserWindow assetsBrowser;
	SceneHierarchyWindow sceneHierarchy;
	PropertyWindow propertyWindow;

	void ProcessMainMenu();
	bool showAboutModalWindow = false;
public:
    void ProcessImgui();
};


void EditorMainWindow::ProcessMainMenu()
{
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New Project","Ctrl+N"))
		{
			auto path = ShellOS::SelectFolderDialog(ShellOS::GetUserDocumentsDirectory());
			if (!path.empty())
			{
				EditorState::gCurrentProject.CreateNew(path.u8string().c_str());
			}
		}

		if (ImGui::MenuItem("Open Project","CTRL+O"))
		{
			auto path = ShellOS::SelectFolderDialog(ShellOS::GetUserDocumentsDirectory());
			if (!path.empty())
			{
				EditorState::gCurrentProject.LoadFromFolder(path.u8string().c_str());
			}
		}

		if (ImGui::MenuItem("Save Project", "CTRL+S"))
		{
		}

		if(ImGui::MenuItem("Save Project As..."))
		{
		}

		if(ImGui::BeginMenu("Recent projects"))
		{
			if(ImGui::MenuItem("Project 1"))
			{
				EditorState::gCurrentProject.LoadFromFolder(u8"D:\\Projects\\mercury\\sample_project");
			}
			if(ImGui::MenuItem("Project 2"))
			{
				EditorState::gCurrentProject.LoadFromFolder(u8"D:\\Projects\\mercury\\new_sample_project");
			}
			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Close", "ALT+F4"))
		{
			EditorState::SetRunning(false);
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Edit"))
	{
		if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
		if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
		ImGui::Separator();
		if (ImGui::MenuItem("Cut", "CTRL+X")) {}
		if (ImGui::MenuItem("Copy", "CTRL+C")) {}
		if (ImGui::MenuItem("Paste", "CTRL+V")) {}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Project"))
	{
		if (ImGui::MenuItem("Build Project", "F7"))
		{
		}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Tools"))
	{
		if (ImGui::MenuItem("Options")) {}
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Help"))
	{
		if (ImGui::MenuItem("About")) {
			showAboutModalWindow = true;
		}
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

void EditorMainWindow::ProcessImgui()
{
	ImGui::PushFont(ImguiState::GetMainFont());
	ProcessMainMenu();

	if (showAboutModalWindow)
	{
		if (ImGui::Begin("About Mercury Editor", &showAboutModalWindow, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Mercury Engine Editor");
			ImGui::Text("Version 0.1.0");
			ImGui::Text("© 2025 Mercury Engine");
			ImGui::Separator();
			if (ImGui::Button("OK", ImVec2(120, 0))) { showAboutModalWindow = false; ImGui::CloseCurrentPopup(); }
			ImGui::End();
		}
	}

	assetsBrowser.ProcessImgui();
	sceneHierarchy.ProcessImgui();
	propertyWindow.ProcessImgui();

	ImGui::PopFont();
}