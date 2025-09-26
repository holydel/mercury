module;

#include "mercury.h"
#include <stdio.h>

#include "ll/os.h"
#include "ll/graphics.h"

export module EditorMain;

import EditorMainWindow;
import ShellOS;
import EditorState;
import ImguiState;

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
    }
    void Initialize() override;
    void Tick() override;
    void Shutdown() override;
    void OnImgui() override;
    bool IsRunning() override { return EditorState::IsRunning(); }
};

EditorApplication gEditor;

void EditorApplication::Initialize() {
    EditorState::gCurrentProject.LoadFromFolder(u8"D:\\Projects\\mercury\\sample_project");


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

void EditorApplication::OnImgui()
{    
    mainWindow.ProcessImgui();
}

void EditorApplication::Shutdown() {
}