#include "ll/os.h"
#ifdef MERCURY_LL_OS_WIN32

#include "application.h"
#include <Windows.h>
#include <iostream>
#include <timeapi.h>
#include "mercury_utils.h"
#include "uwp_input.h"
#include "mercury_log.h"
#include <thread>

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include "ll/graphics/vulkan/vk_swapchain.h"
#endif

#ifdef MERCURY_LL_GRAPHICS_WEBGPU
#include "ll/graphics/webgpu/webgpu_graphics.h"
#endif

#include "../../../imgui/imgui_impl.h"

#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "Winmm.lib")

HINSTANCE gWinSystemInstance = nullptr;
HWND gMainWindow = nullptr;
SYSTEM_INFO gSysInfo;
constexpr const wchar_t* winClassName = L"MercuryWindow";


namespace mercury::ll::os
{
  const OSInfo &OS::GetInfo() { 
    static OSInfo info{OSType::Windows, OSArchitecture::x64};
    return info; 
  }

  void OS::Initialize() { 
    std::cout << "OS::Initialize (UWP)" << std::endl;
  }

  void OS::Shutdown() {
    std::cout << "OS::Shutdown (UWP)" << std::endl;
  }

  bool OS::IsFocused() { return true; }

  void OS::Sleep(u32 milliseconds) {
    ::Sleep(milliseconds);
  }

  void* OS::GetCurrentNativeWindowHandle() {
    return gMainWindow;
  }

  void OS::CreateNativeWindow(NativeWindowDescriptor &desc)
  {
	  //cannot create window in UWP
  }

  void OS::DestroyNativeWindow()
  {

  }

  void OS::SetNativeWindowTitle(const c8string &title)
  {

  }

  void OS::SetNativeWindowSize(int width, int height)
  {

  }

  void OS::SetNativeWindowFullscreen(bool fullscreen)
  {

  }

  bool OS::IsNativeWindowFullscreen()
  {
      return false;
  }

  void OS::Update()
  {

  }

  void OS::FatalFail(const char* reason)
  {
    MLOG_FATAL(u8"Fatal error: %s", reason);
    DebugBreak();
  }


  void* OS::LoadSharedLibrary(const c8* utf8libname)
  {
    wchar_t libnameW[1024] = {0};
    utils::string::utf8_to_utf16(utf8libname, (char16_t*)libnameW, 1024);
    
    HMODULE libHandle = ::LoadLibraryW(libnameW);
    if (libHandle == nullptr)
    {
      DWORD error = GetLastError();
      MLOG_ERROR(u8"Failed to load shared library: %s, error code: %lu", utf8libname, error);
      return nullptr;
    }

    return libHandle;
  }

	c8string OS::GetSharedLibraryFullFilename(void* libHandle)
  {
    if (libHandle == nullptr)
    {
      return c8string();
    }

    HMODULE hModule = static_cast<HMODULE>(libHandle);
    wchar_t filename[MAX_PATH] = { 0 };
    GetModuleFileNameW(hModule, filename, MAX_PATH);

    c8 filenameU8[1024] = { 0 };
    utils::string::utf16_to_utf8((const char16_t*)filename, filenameU8, 1024);
    return c8string(filenameU8);
  }

	bool OS::UnloadSharedLibrary(void* library)
  {
    return FreeLibrary(static_cast<HMODULE>(library));
  }

	void* OS::GetFuncPtrImpl(void* library, const char* funcName)
  {
    	return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(library), funcName));
  }

  size_t OS::GetPageSize()
  { 
    //dwPageSize returns internal system page size but we interested in the sizes for alloc / commit pages there
    return gSysInfo.dwAllocationGranularity;
  }

  void* OS::ReserveMemory(size_t size)
  {
    void* ptr = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    return ptr;
  }

  void OS::CommitMemory(void* ptr, size_t size)
  {
    VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
  }

  void OS::DecommitMemory(void* ptr, size_t size)
  {
    VirtualFree(ptr, size, MEM_DECOMMIT);
  }

  void OS::ReleaseMemory(void* ptr, size_t size)
  {
    VirtualFree(ptr, size, MEM_RELEASE);
  }

  const char* OS::GetName()
  {
    static const char* osName = "Windows";
    return osName;
  }

  bool OS::IsNativeWindowReady()
  {
        return gMainWindow != nullptr;
  }

  void OS::GetActualWindowSize(unsigned int& widthOut, unsigned int& heightOut)
  {
          widthOut = 0;
          heightOut = 0;
  }

  void OS::ImguiInitialize()
  {
	  ImGui_ImplUwp_Init(gMainWindow);
  }
  void OS::ImguiNewFrame()
  {
      ImGui_ImplUwp_NewFrame();
  }
  void OS::ImguiShutdown()
  {
      ImGui_ImplUwp_Shutdown();
  }

#ifdef MERCURY_LL_GRAPHICS_VULKAN
  void* OS::CreateVkSurface(void* vk_instance, void* allocations_callback)
  {
	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = static_cast<HWND>(gMainWindow);
	createInfo.hinstance = static_cast<HINSTANCE>(GetModuleHandleA(nullptr));

	VK_CALL(vkCreateWin32SurfaceKHR(gVKInstance, &createInfo, (VkAllocationCallbacks*)allocations_callback, &gVKSurface));
    return gVKSurface;
  }
 
  bool OS::IsQueueSupportPresent(void* vk_physical_device, u32 queueIndex)
  {
    bool supportPresent = vkGetPhysicalDeviceWin32PresentationSupportKHR != nullptr ? vkGetPhysicalDeviceWin32PresentationSupportKHR((VkPhysicalDevice)vk_physical_device, queueIndex) : false;
    return supportPresent;
  }
#endif

#ifdef MERCURY_LL_GRAPHICS_WEBGPU
  wgpu::Surface* gWebGPUSurfacePtr = nullptr;
  void* OS::GetWebGPUNativeWindowHandle()
  {
      // Create WebGPU surface descriptor for Windows
      wgpu::SurfaceDescriptorFromWindowsHWND windowsDesc = {};
      windowsDesc.sType = wgpu::SType::SurfaceSourceWindowsHWND;
      windowsDesc.hwnd = gMainWindow;
      windowsDesc.hinstance = GetModuleHandle(nullptr);

      wgpu::SurfaceDescriptor surfaceDesc = {};
      surfaceDesc.nextInChain = &windowsDesc;
      surfaceDesc.label = "Main Window WGPU Surface";

      // Create the surface using the global WebGPU instance
      wgpu::Surface surface = wgpuInstance.CreateSurface(&surfaceDesc);

      if (!surface) {
          MLOG_ERROR(u8"Failed to create WebGPU surface");
          return nullptr;
      }

      MLOG_DEBUG(u8"WebGPU Win32 surface created successfully");

      // Allocate and return the surface pointer
      // The surface needs to be heap-allocated to persist beyond this function
      gWebGPUSurfacePtr = new wgpu::Surface(surface);
      return gWebGPUSurfacePtr;
  }
  void OS::WebGPUPresent()
  {
      if (gWebGPUSurfacePtr)
          gWebGPUSurfacePtr->Present();
  }
#endif
  OS* gOS = nullptr;
}
// entry point for win32


#endif