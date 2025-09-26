module;

#include <imgui.h>
#include <ll/graphics.h>
export module ImguiState;

export namespace ImguiState {
	
	ImFont* mainFont = nullptr;
	ImFont* shaderFont = nullptr;

	export void PrepareImgui()
	{
		ImGuiIO& io = ImGui::GetIO();
		
		shaderFont = io.Fonts->AddFontFromFileTTF("FiraCode-SemiBold.ttf", 18.0f);
		mainFont = io.Fonts->AddFontFromFileTTF("Roboto-Regular.ttf", 18.0f);

		
		mercury::ll::graphics::gDevice->ImguiRegenerateFontAtlas();
	}

	export ImFont* GetMainFont()
	{
		return mainFont;
	}

	export ImFont* GetShaderFont()
	{
		return shaderFont;
	}
}