module;

#include <imgui.h>
#include <ll/graphics.h>
#include "IconsFontAwesome7.h"

export module ImguiState;

export namespace ImguiState {
	
	ImFont* mainFont = nullptr;
	ImFont* shaderFont = nullptr;

	export void PrepareImgui()
	{
		ImGuiIO& io = ImGui::GetIO();
		
		shaderFont = io.Fonts->AddFontFromFileTTF("FiraCode-SemiBold.ttf", 18.0f);


		mainFont = io.Fonts->AddFontFromFileTTF("Roboto-Regular.ttf", 18.0f);

		ImFontConfig config;
		config.MergeMode = true;
		config.GlyphMinAdvanceX = 20.0f;

		static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

		
		io.Fonts->AddFontFromFileTTF("Font Awesome 7 Free-Regular-400.otf", 18.0f, &config, icon_ranges);
		io.Fonts->AddFontFromFileTTF("Font Awesome 7 Free-Solid-900.otf", 18.0f, &config, icon_ranges);
		io.Fonts->Build();
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