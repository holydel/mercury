module;
#include <imgui.h>

export module SceneHierarchyWindow;

export class SceneHierarchyWindow
{
public:
	static SceneHierarchyWindow& Get() {
		static SceneHierarchyWindow instance;
		return instance;
	}
	void ProcessImgui();
};

void SceneHierarchyWindow::ProcessImgui()
{
	ImGui::Begin("Scene");
	
	
	ImGui::End();
}