module;
#include <imgui.h>

export module SceneHierarchyWindow;

export class SceneHierarchyWindow
{
public:
	void ProcessImgui();
};

void SceneHierarchyWindow::ProcessImgui()
{
	ImGui::Begin("Scene");
	
	
	ImGui::End();
}