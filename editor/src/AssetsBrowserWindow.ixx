module;
#include <imgui.h>

export module AssetsBrowserWindow;

export class AssetsBrowserWindow
{
public:
	void ProcessImgui();
};

void AssetsBrowserWindow::ProcessImgui()
{
	ImGui::Begin("Assets Browser");
	ImGui::Text("Assets Browser Content Here");
	ImGui::End();
}