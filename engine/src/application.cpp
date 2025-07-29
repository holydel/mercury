#include "application.h"

using namespace mercury;

static Application *g_currentApplication = nullptr;

Application::Application() { g_currentApplication = this; }

Application::~Application() { g_currentApplication = nullptr; }

void TickCurrentApplication() { g_currentApplication->Tick(); }

void InitializeCurrentApplication() { g_currentApplication->Initialize(); }

void ShutdownCurrentApplication() { g_currentApplication->Shutdown(); }

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
