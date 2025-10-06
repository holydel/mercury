module;
#include <imgui.h>
#include "ll/graphics.h"
export module PropertyWindow;

export class IImGuiPropertyDrawer
{
public:
	virtual void DrawProperty() {
		ImGui::Text("Base property drawer");
	}

	virtual void FinalPassPreview(mercury::ll::graphics::CommandList& finalCL)
	{

	}
};

export class PropertyWindow
{
	IImGuiPropertyDrawer* objectToEdit = nullptr;
public:
	static PropertyWindow& Get() {
		static PropertyWindow instance;
		return instance;
	}
	void ProcessImgui();
	void ProcessFinalPass(mercury::ll::graphics::CommandList& finalCL);
	void SetObjectToEdit(IImGuiPropertyDrawer* obj) { objectToEdit = obj; }
};

void PropertyWindow::ProcessImgui()
{
	ImGui::Begin("Properties");
	ImGui::Text("Test property");

	if (objectToEdit) {
		objectToEdit->DrawProperty();
	} else {
		ImGui::TextDisabled("No object selected.");
	}
	ImGui::End();
}

void PropertyWindow::ProcessFinalPass(mercury::ll::graphics::CommandList& finalCL)
{
	if (objectToEdit)
	{
		objectToEdit->FinalPassPreview(finalCL);
	}
}