#include "ll/os.h"
#ifdef MERCURY_LL_OS_EMSCRIPTEN
#include "mercury_log.h"
#include "application.h"
#include <emscripten.h>
#include <iostream>

#ifdef MERCURY_LL_GRAPHICS_WEBGPU
#include "ll/graphics/webgpu/webgpu_graphics.h"
#endif

#include "../../../imgui/imgui_impl.h"

#include "emscripten_input.h"
#include <emscripten/html5.h>
#include <emscripten.h>

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

float emMouseX;
float emMouseY;
bool emMouseDown[5];

wgpu::Surface gWgpuSurface;

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
    if(gWgpuSurface)
    {
      return gWgpuSurface.Get();
    }
    return nullptr;
  }

  void OS::CreateNativeWindow(NativeWindowDescriptor& desc) {
    // Emscripten doesn't need to create windows - they're provided by the browser
  }

  void OS::Update() {
    // Emscripten main loop is handled by emscripten_set_main_loop
  }

  void OS::GetActualWindowSize(unsigned int& width, unsigned int& height) {
    // Get actual canvas display size from browser
    EM_ASM({
      var canvas = document.getElementById('canvas');
      if (canvas) {
        var rect = canvas.getBoundingClientRect();
        setValue($0, rect.width, 'i32');
        setValue($1, rect.height, 'i32');
      } else {
        // Fallback to viewport size
        setValue($0, window.innerWidth, 'i32');
        setValue($1, window.innerHeight, 'i32');
      }
    }, &width, &height);
  }

  void OS::ImguiInitialize() {
    ImGui_ImplEmscripten_Init();
  }

  void OS::ImguiNewFrame() {
    	// ImGui temporarily disabled for Emscripten
      int width = 0;
      int height = 0;
      emscripten_get_canvas_element_size("#canvas", &width, &height);
      ImGui::GetIO().DisplaySize = ImVec2((float)width, (float)height);
	  ImGui_ImplEmscripten_Event();
    ImGui_ImplEmscripten_NewFrame();
  }

  void OS::ImguiShutdown() {
    ImGui_ImplEmscripten_Shutdown();
  }

  const char* OS::GetName() {
    return "Emscripten";
  }

#ifdef MERCURY_LL_GRAPHICS_WEBGPU
  void* OS::GetWebGPUNativeWindowHandle() {
    // Function to get canvas surface

    MLOG_DEBUG(u8"Getting canvas surface...");

    int gInitialCanvasWidth = 0;
    int gInitialCanvasHeight = 0;
    // Get the canvas element
    EMSCRIPTEN_RESULT result = emscripten_get_canvas_element_size("#canvas", &gInitialCanvasWidth, &gInitialCanvasHeight);
    if (result != EMSCRIPTEN_RESULT_SUCCESS)
    {
        MLOG_ERROR(u8"Failed to get canvas element");
        return nullptr;
    }

    // Create surface from canvas using Emscripten-specific types
    wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc;
    canvasDesc.selector = "canvas";

    wgpu::SurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = &canvasDesc;

    gWgpuSurface = wgpuInstance.CreateSurface(&surfaceDesc);
    if (gWgpuSurface)
    {
        MLOG_DEBUG(u8"Canvas surface created successfully");
    }
    else
    {
        MLOG_ERROR(u8"Failed to create canvas surface");
    }

    return &gWgpuSurface;
  }

  void OS::WebGPUPresent() {
    // Emscripten WebGPU present is handled automatically
  }
#endif

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