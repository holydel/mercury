#include "editor_main_window.h"
#include <imgui.h>
#include <cstdlib> 

import ShellOS;

void EditorMainWindow::DrawImgui()
{
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New Project"))
		{
			ShellOS::SelectFolderDialog(ShellOS::GetUserDocumentsDirectory());
		}

		if (ImGui::MenuItem("Close"))
		{
			exit(0);
		}

		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();

	ImGui::SetNextWindowSize(ImVec2(500, 300));
	ImGui::Begin("TestWindow");
	ImGui::LabelText("FooBar", "Format");
	ImGui::End();
}