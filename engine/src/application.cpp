#include "application.h"

#include "ll/os.h"
//#include "ll/graphics.h"
#include "ll/physic.h"
#include "ll/sound.h"
#include "ll/xr.h"

#include "graphics.h"

using namespace mercury;

static Application *g_currentApplication = nullptr;

Application::Application() { g_currentApplication = this; }

Application::~Application() { g_currentApplication = nullptr; }

void TickCurrentApplication() {
  g_currentApplication->Tick();

  TickGraphics();
}

void InitializeCurrentApplication() { 
  ll::os::gOS = new ll::os::OS();
  ll::os::gOS->Initialize();

  InitializeGraphics();
  
  g_currentApplication->Initialize();
}

void ShutdownCurrentApplication() {
  g_currentApplication->Shutdown();
  
  ShutdownGraphics();
  
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
