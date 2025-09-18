#include "mercury.h"
#include <stdio.h>

#include "ll/os.h"
#include "ll/graphics.h"

#include "editor_main_window.h"

using namespace mercury;

class EditorApplication : public Application {
    bool m_running = true;

    EditorMainWindow mainWindow;
public:
       void Configure() override
       {
              config.appName = u8"Mercury Engine Editor";
              config.appVersion = Config::Version{{{0, 1, 0}}};
              config.window.width = 1600;
              config.window.height = 900;
              config.window.resizable = true;
              config.graphics.enableValidationLayers = true; // Enable validation layers for testing
              config.graphics.adapterPreference = Config::Graphics::AdapterTypePreference::HighPerformance;
       }
       void Initialize() override;
       void Tick() override;
       void Shutdown() override;
       void OnImgui() override;
       bool IsRunning() override { return m_running; }
};

EditorApplication gEditor;

void EditorApplication::Initialize() {

}

void EditorApplication::Tick() {
      if(ll::graphics::gSwapchain)
      {
        ll::graphics::gSwapchain->clearColor = glm::vec4(0.02f,0.03f,0.05f,1.0f);
      }      

      if(input::gKeyboard->IsKeyPressed(input::Key::Escape)) {
              MLOG_DEBUG(u8"Escape key pressed, shutting down application.");
              m_running = false;
      }

}

void EditorApplication::OnImgui()
{
    mainWindow.DrawImgui();
}

void EditorApplication::Shutdown() {
}