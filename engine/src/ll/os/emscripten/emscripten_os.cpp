#include "ll/os.h"
#ifdef MERCURY_LL_OS_EMSCRIPTEN

#include "application.h"
#include <emscripten.h>
#include <iostream>

#include "emscripten_input.h"
// Export C functions for JavaScript access
EMSCRIPTEN_KEEPALIVE
extern "C" void onPageHidden();

EMSCRIPTEN_KEEPALIVE
extern "C" void onPageVisible();

EMSCRIPTEN_KEEPALIVE
extern "C" void requestApplicationShutdown();

 // Global state for main loop management
static bool g_applicationRunning = true;
static bool g_pageVisible = true;

extern void* gWebGPUSurface;
namespace mercury::ll::os
{
  const OSInfo &OS::GetInfo() { 
    static OSInfo info{OSType::Emscripten, OSArchitecture::x64};
    return info; 
  }

  void OS::Initialize() { 
    std::cout << "OS::Initialize (Emscripten)" << std::endl;
  }

  void OS::Shutdown() {
    std::cout << "OS::Shutdown (Emscripten)" << std::endl;
  }

  bool OS::IsFocused() { return g_pageVisible; }

  void OS::Sleep(u32 milliseconds) {
    emscripten_sleep(milliseconds);
  }

  void* OS::GetCurrentNativeWindowHandle() {
    #ifdef MERCURY_LL_GRAPHICS_WEBGPU
    return gWebGPUSurface;
    #else
    return nullptr;
    #endif
  }
  OS* gOS = nullptr;
} 


// Browser visibility change callback
EM_JS(void, setupVisibilityCallback, (), {
  document.addEventListener('visibilitychange', function() {
    if (document.hidden) {
      // Page is hidden - pause or slow down
      Module.ccall('onPageHidden', 'v');
    } else {
      // Page is visible - resume normal operation
      Module.ccall('onPageVisible', 'v');
    }
  });
});

// Called when page becomes hidden
extern "C" EMSCRIPTEN_KEEPALIVE void onPageHidden()
{
  g_pageVisible = false;
}

// Called when page becomes visible
extern "C" EMSCRIPTEN_KEEPALIVE void onPageVisible()
{
  g_pageVisible = true;
}

// Main loop callback function
void emscriptenMainLoop()
{
  // Check if application should continue running
  if (!g_applicationRunning)
  {
    // Application requested shutdown
    ShutdownCurrentApplication();
    emscripten_cancel_main_loop();
    return;
  }
  
  TickCurrentApplication();

  // Check if the application itself wants to stop running
  if (!mercury::Application::GetCurrentApplication()->IsRunning())
  {
    g_applicationRunning = false;
  }
}

// Function to request application shutdown (can be called from JavaScript)
extern "C" EMSCRIPTEN_KEEPALIVE void requestApplicationShutdown()
{
  g_applicationRunning = false;
}

int main()
{
  // Initialize the application first
  InitializeCurrentApplication();
  
  // Set up browser visibility handling
  setupVisibilityCallback();
  
  RegisterEmscriptenInputCallbacks();
  
  // Set up the main loop with optimal parameters:
  // - fps: 0 (unlimited, browser will throttle appropriately)
  // - simulate_infinite_loop: 1 (true, prevents return from main)
  // - arg: nullptr (no additional data needed)
  emscripten_set_main_loop(emscriptenMainLoop, 0, 1);
  
  // This line should never be reached due to simulate_infinite_loop=1
  return 0;
}
#endif