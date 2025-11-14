module;

#include "mercury.h"
#include <stdio.h>

#include "ll/os.h"
#include "ll/graphics.h"

export module EditorMain;

import EditorMainWindow;
import ShellOS;
import EditorState;
import EditorOptions;
import ImguiState;
import ShaderCompiler;
import PropertyWindow;
import AssetsLoadersRegistry;

using namespace mercury;


class EditorApplication : public Application {
    EditorMainWindow mainWindow;
public:
    void Configure() override
    {
        config.appName = u8"Mercury Engine Editor";
        config.appVersion = Config::Version{ {{0, 1, 0}} };
        config.window.width = 1600;
        config.window.height = 900;
        config.window.resizable = true;
        config.graphics.enableValidationLayers = true; // Enable validation layers for testing
        config.graphics.adapterPreference = Config::Graphics::AdapterTypePreference::HighPerformance;

        config.imgui.enableDocking = true;
        config.imgui.enableViewports = true;

		
        EditorOptions::Initialize();

        ShaderCompiler::Initialize(); //initialize as early as possible
    }
    void Initialize() override;
    void Tick() override;
    void Shutdown() override;
    void OnImgui() override;
    void OnFinalPass(ll::graphics::CommandList &finalCL) override;
    bool IsRunning() override { return EditorState::IsRunning(); }
};

EditorApplication gEditor;

void EditorApplication::Initialize() {
    AssetsLoadersRegistry::GetInstance().Initialize();
    EditorState::gCurrentProject.LoadFromFolder(u8"D:\\Projects\\mercury\\testbed_project");


    ImguiState::PrepareImgui();
    //currentProject.CreateNew(u8"D:\\Projects\\mercury\\new_sample_project");
}

void EditorApplication::Tick() {
    if (ll::graphics::gSwapchain)
    {
        ll::graphics::gSwapchain->clearColor = glm::vec4(0.02f, 0.03f, 0.05f, 1.0f);
    }

    if (input::gKeyboard->IsKeyPressed(input::Key::Escape)) {
        MLOG_DEBUG(u8"Escape key pressed, shutting down application.");
		EditorState::SetRunning(false);
    }

}

void EditorApplication::OnFinalPass(mercury::ll::graphics::CommandList& finalCL)
{
	PropertyWindow::Get().ProcessFinalPass(finalCL);
}

void EditorApplication::OnImgui()
{    
    mainWindow.ProcessImgui();
}

void EditorApplication::Shutdown() {
    
	ShaderCompiler::Shutdown();
}