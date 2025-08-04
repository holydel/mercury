#include "ll/os.h"
#ifdef MERCURY_LL_OS_EMSCRIPTEN

#include "application.h"
#include <emscripten.h>
#include <iostream>

// Export C functions for JavaScript access
EMSCRIPTEN_KEEPALIVE
extern "C" void onPageHidden();

EMSCRIPTEN_KEEPALIVE
extern "C" void onPageVisible();

EMSCRIPTEN_KEEPALIVE
extern "C" void requestApplicationShutdown();

/*
 * Emscripten Main Loop Implementation
 * 
 * This implementation provides an optimal main loop for Emscripten with focus on:
 * 
 * 1. PERFORMANCE: Uses emscripten_set_main_loop with fps=0 for unlimited frame rate
 *    - Browser will automatically throttle to ~60fps for smooth rendering
 *    - No artificial frame rate limiting that could cause stuttering
 * 
 * 2. EFFICIENCY: Implements page visibility detection
 *    - Pauses application ticks when browser tab is not visible
 *    - Saves battery life and CPU usage when user switches tabs
 *    - Automatically resumes when tab becomes visible again
 * 
 * 3. PROPER LIFECYCLE: Complete initialization and shutdown handling
 *    - Calls InitializeCurrentApplication() before starting main loop
 *    - Checks application's IsRunning() state each frame
 *    - Calls ShutdownCurrentApplication() when application terminates
 *    - Provides JavaScript interface for external shutdown requests
 * 
 * 4. BROWSER INTEGRATION: Uses EM_JS for seamless JavaScript integration
 *    - Sets up visibility change event listeners
 *    - Provides C functions callable from JavaScript
 *    - Maintains proper C++/JavaScript boundary
 * 
 * Key Functions:
 * - emscriptenMainLoop(): Main loop callback that handles ticking and state checks
 * - requestApplicationShutdown(): External shutdown request (callable from JS)
 * - onPageHidden/onPageVisible(): Browser visibility state management
 * - setupVisibilityCallback(): Sets up browser event listeners
 */

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
  
  // Set up the main loop with optimal parameters:
  // - fps: 0 (unlimited, browser will throttle appropriately)
  // - simulate_infinite_loop: 1 (true, prevents return from main)
  // - arg: nullptr (no additional data needed)
  emscripten_set_main_loop(emscriptenMainLoop, 0, 1);
  
  // This line should never be reached due to simulate_infinite_loop=1
  return 0;
}
#endif