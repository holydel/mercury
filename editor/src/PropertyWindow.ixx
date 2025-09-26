module;
#include <imgui.h>

export module PropertyWindow;

export class PropertyWindow
{
public:
	void ProcessImgui();
};

void PropertyWindow::ProcessImgui()
{
	ImGui::Begin("Properties");
	ImGui::Text("Test property");
	ImGui::End();
}