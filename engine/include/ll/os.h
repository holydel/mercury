#pragma once

#include "../mercury_api.h"
#include <string>

namespace mercury {
namespace ll {
namespace os {
enum class OSType : u8 {
  Windows,
  Linux,
  MacOS,
  Emscripten,
  Android,
};

enum class OSArchitecture : u8 {
  x64,
  ARM64,
};

struct OSInfo {
  OSType type;
  OSArchitecture architecture;
};

class OS {
public:
  OS() = default;
  ~OS() = default;

  void Initialize();
  void Shutdown();

  const OSInfo &GetInfo();

  bool IsFocused();

  void Sleep(u32 milliseconds);

  /// @brief Some platforms (android) may destroy the native window handle during runtime.
  /// This function should return the current native window handle.
  /// @return 
  void* GetCurrentNativeWindowHandle();

  struct NativeWindowDescriptor {
    std::u8string title;
    int width;
    int height;

    bool resizable;
    bool fullscreen;
    bool maximized;
  };

  void CreateNativeWindow(NativeWindowDescriptor &desc);
  void DestroyNativeWindow();
  void SetNativeWindowTitle(const std::u8string &title);
  void SetNativeWindowSize(int width, int height);
  void SetNativeWindowFullscreen(bool fullscreen);  
  bool IsNativeWindowFullscreen();

  void Update();

  void FatalFail(const char* reason);

  void* LoadSharedLibrary(const char8_t* utf8libname);
	std::u8string GetSharedLibraryFullFilename(void* libHandle);
	bool UnloadSharedLibrary(void* library);

	void* GetFuncPtrImpl(void* library, const char* funcName);

	template <typename T>
	T GetFuncPtr(void* library, const char* funcName)
	{
		return reinterpret_cast<T>(GetFuncPtrImpl(library, funcName));
	}

  size_t GetPageSize();
  void* ReserveMemory(size_t size);
  void ReleaseMemory(void* ptr, size_t size);
  void CommitMemory(void* ptr, size_t size);
  void DecommitMemory(void* ptr, size_t size);

  const char* GetName();

  bool IsNativeWindowReady(); //the window may be created and GetCurrentNativeWindowHandle return not null, but some surfaces can be not ready yet. so we need to wait
  void GetActualWindowSize(unsigned int& widthOut, unsigned int& heightOut);
  void ImguiInitialize();
  void ImguiNewFrame();
  void ImguiShutdown();

  #ifdef MERCURY_LL_GRAPHICS_VULKAN
  void* CreateVkSurface(void* vk_instance,void* allocations_callback); //return VkSurface
  bool IsQueueSupportPresent(void* vk_physical_device, u32 queueIndex);
  #endif

#ifdef MERCURY_LL_GRAPHICS_WEBGPU
  void* GetWebGPUNativeWindowHandle(); //return EmscriptenWebGPUContext*
  void WebGPUPresent();
#endif
};

extern OS *gOS;

#define LOAD_FUNC_BY_NAME(libHandle,funcPtrOut,funcName) funcPtrOut = mercury::ll::os::gOS->GetFuncPtr<decltype(funcPtrOut)>(libHandle,funcName);

#define LOAD_FUNC_PTR(libHandle,funcPtrOut) LOAD_FUNC_BY_NAME(libHandle,funcPtrOut,#funcPtrOut)

#define MERCURY_ASSERT(assert) if(!(assert)) {mercury::ll::os::gOS->FatalFail(#assert);}

} // namespace os
} // namespace ll
} // namespace mercury