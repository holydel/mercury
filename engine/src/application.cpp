#include "application.h"

#include "ll/os.h"
//#include "ll/graphics.h"
#include "ll/physic.h"
#include "ll/sound.h"
#include "ll/xr.h"
#include "input.h"
#include "ll/graphics/ll_graphics.h"
#include <iostream>
#include <chrono>

using namespace mercury;

static Application *g_currentApplication = nullptr;

Application::Application() { g_currentApplication = this; }

Application::~Application() { g_currentApplication = nullptr; }

void TickCurrentApplication() {

  mercury::ll::os::gOS->Update();

  MercuryInputPreTick();

  g_currentApplication->Tick();

  MercuryInputPostTick();

  MercuryGraphicsTick();
}

void CreateMainWindow(const Config &appCfg) {

  ll::os::OS::NativeWindowDescriptor desc;
  desc.title = appCfg.appName;  
  desc.width = appCfg.window.width;
  desc.height = appCfg.window.height;
  desc.resizable = appCfg.window.resizable;
  desc.fullscreen = appCfg.window.fullscreen;
  desc.maximized = appCfg.window.maximized;

  ll::os::gOS->CreateNativeWindow(desc);
}

void InitializeCurrentApplication() { 

  auto os = ll::os::gOS = new ll::os::OS();

  g_currentApplication->Configure();
  const auto &appCfg = g_currentApplication->GetConfig();

  os->Initialize();

  //create or obtain native window
  if(!appCfg.window.headlessMode) {
    CreateMainWindow(appCfg);
  }

  MercuryInputInitialize();

  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
  MercuryGraphicsInitialize();
  std::cout << "Graphics initialisation time: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() << " ms" << std::endl;

  g_currentApplication->Initialize();
}

void ShutdownCurrentApplication() {
  g_currentApplication->Shutdown();
  
  MercuryGraphicsShutdown();
  MercuryInputShutdown();
  
  ll::os::gOS->Shutdown();
  delete ll::os::gOS;
}

Application *Application::GetCurrentApplication() { return g_currentApplication; }

void RunCurrentApplication() {
  InitializeCurrentApplication();

  // initialize os
  // initialize graphics
  // initialize sound
  while (g_currentApplication->IsRunning()) {
    TickCurrentApplication();
  }
  ShutdownCurrentApplication();
}
