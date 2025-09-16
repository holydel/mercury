#include "ll/os.h"
#ifdef MERCURY_LL_OS_LINUX

#include "application.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include "mercury_utils.h"
#include "linux_input.h"
#include "mercury_log.h"
#include <sys/mman.h>

#include "linux_display_server.h"

#include <cstring>

LinuxDisplayServer *gDisplayServer = nullptr;


namespace mercury::ll::os
{
  const OSInfo &OS::GetInfo() { 
    static OSInfo info{OSType::Linux, OSArchitecture::x64};
    return info; 
  }

  void OS::Initialize() { 
    std::cout << "OS::Initialize (Linux)" << std::endl;
  }

  void OS::Shutdown() {
    std::cout << "OS::Shutdown (Linux)" << std::endl;
  }

  bool OS::IsFocused() { 
    return true; 
  }

  void OS::Sleep(u32 milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
  }

  void* OS::GetCurrentNativeWindowHandle() {
    IF_UNLIKELY(!gDisplayServer)
      return nullptr;

    return gDisplayServer->GetWindowNativeHandle();
  }

  void OS::CreateNativeWindow(NativeWindowDescriptor &desc)
  {
    auto& config = Application::GetCurrentApplication()->GetConfig();
    auto& winCfg = config.window;

    gDisplayServer = LinuxDisplayServer::CreateWayland();
    gDisplayServer->Initialize();
    gDisplayServer->CreateWindow(desc);
  }

  void OS::DestroyNativeWindow(void* native_window_handle)
  {
    IF_LIKELY(gDisplayServer)
    {
      gDisplayServer->DestroyWindow();
      delete gDisplayServer;
    }      
  }

  void OS::GetActualWindowSize(unsigned int& widthOut, unsigned int& heightOut)
  {
    IF_LIKELY(gDisplayServer)
    {
      gDisplayServer->GetActualWindowSize(widthOut, heightOut);
    }      
  }

  void OS::SetNativeWindowTitle(void* native_window_handle, const std::u8string &title)
  {

  }

  void OS::SetNativeWindowSize(void* native_window_handle, int width, int height)
  {
 
  }

  void OS::SetNativeWindowFullscreen(void* native_window_handle, bool fullscreen)
  {
    
  }

  bool OS::IsNativeWindowFullscreen(void* native_window_handle)
  {
    return false;
  }

  void OS::ImguiInitialize()
  {
    IF_LIKELY(gDisplayServer)
      gDisplayServer->ImguiInitialize();
  }

  void OS::ImguiNewFrame()
  {
    IF_LIKELY(gDisplayServer)
      gDisplayServer->ImguiNewFrame();
  }

  void OS::ImguiShutdown()
  {
    IF_LIKELY(gDisplayServer)
      gDisplayServer->ImguiShutdown();
  }

  void OS::Update()
  {
    gDisplayServer->ProcessEvents();
  }

  void OS::FatalFail(const char* reason)
  {
    MLOG_FATAL(u8"Fatal error: %s", reason);
    exit(EXIT_FAILURE);
  }

  void* OS::LoadSharedLibrary(const char8_t* utf8libname)
  {
    const char* libname = reinterpret_cast<const char*>(utf8libname);
    void* handle = dlopen(libname, RTLD_LAZY);
    
    if (!handle) {
        MLOG_ERROR(u8"Failed to load shared library: %s, error: %s", 
                  utf8libname, dlerror());
        return nullptr;
    }
    
    return handle;
  }

  std::u8string OS::GetSharedLibraryFullFilename(void* libHandle)
  {
    IF_UNLIKELY (!libHandle) {
        return std::u8string();
    }
    
    // On Linux, getting the full path of a loaded library is more complex
    // This is a simplified implementation that might not work in all cases
    Dl_info info;
    if (dladdr(libHandle, &info)) {
        return std::u8string(reinterpret_cast<const char8_t*>(info.dli_fname));
    }
    
    MLOG_ERROR(u8"Failed to get library filename");
    return std::u8string();
  }

  bool OS::UnloadSharedLibrary(void* library)
  {
    return dlclose(library) == 0;
  }

  void* OS::GetFuncPtrImpl(void* library, const char* funcName)
  {
    return dlsym(library, funcName);
  }

  size_t OS::GetPageSize()
  { 
    return sysconf(_SC_PAGESIZE);
  }

  void* OS::ReserveMemory(size_t size)
  {
    // On Linux, we can use mmap with PROT_NONE to reserve memory
    void* ptr = mmap(nullptr, size, PROT_NONE, 
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    IF_UNLIKELY (ptr == MAP_FAILED) {
        return nullptr;
    }
    return ptr;
  }

  void OS::CommitMemory(void* ptr, size_t size)
  {
    // Change protection to read/write
    mprotect(ptr, size, PROT_READ | PROT_WRITE);
  }

  void OS::DecommitMemory(void* ptr, size_t size)
  {
    // Change protection to none
    mprotect(ptr, size, PROT_NONE);
  }

  void OS::ReleaseMemory(void* ptr, size_t size)
  {
    munmap(ptr, size);
  }

  const char* OS::GetName()
  {
    static const char* osName = "Linux";
    return osName;
  }

  bool OS::IsNativeWindowReady()
  {
    IF_UNLIKELY(!gDisplayServer)
      return false;

    return gDisplayServer->IsNativeWindowReady();
  }
  #ifdef MERCURY_LL_GRAPHICS_VULKAN
  void* OS::CreateVkSurface(void* vk_instance,void* allocations_callback)
  {
    IF_UNLIKELY(!gDisplayServer)
      return nullptr;

      return gDisplayServer->CreateSurface(vk_instance, allocations_callback);
  }
 
  bool OS::IsQueueSupportPresent(void* vk_physical_device, u32 queueIndex)
  {
    IF_UNLIKELY(!gDisplayServer)
      return false;

      return gDisplayServer->CheckIfPresentSupportedOnQueue(vk_physical_device, queueIndex);
  }
  #endif

  OS* gOS = nullptr;
} 

// Entry point for Linux
int main(int argc, char **argv) {
    // Set up locale for UTF-8 support
    setlocale(LC_ALL, "en_US.UTF-8");
    
    std::cout << "Linux X11 application starting" << std::endl;
    
    // Initialize and run the application
    RunCurrentApplication();
    
    return 0;
}

#endif