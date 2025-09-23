#include "ll/os.h"
#ifdef MERCURY_LL_OS_WIN32

#include "application.h"
#include <Windows.h>
#include <iostream>
#include <timeapi.h>
#include "mercury_utils.h"
#include "win32_input.h"
#include "mercury_log.h"
#include <thread>

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include "ll/graphics/vulkan/vk_swapchain.h"
#endif

#include "../../../imgui/imgui_impl.h"

#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "Winmm.lib")

HINSTANCE gWinSystemInstance = nullptr;
HWND gMainWindow = nullptr;
SYSTEM_INFO gSysInfo;
constexpr const wchar_t* winClassName = L"MercuryWindow";

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	 extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	 if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam))
	 	return true;

	switch (message)
	{
	case WM_INPUT_DEVICE_CHANGE:
	{
		int c = 42;
	}
	break;
	
	// Mouse messages
	case WM_MOUSEMOVE:
		HandleMouseMove(wParam, lParam);
		break;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
		HandleMouseButton(message, wParam, lParam, true);
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
		HandleMouseButton(message, wParam, lParam, false);
		break;
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDBLCLK:
		printf("Mouse double click message received\n");
		HandleMouseDoubleClick(message, wParam, lParam);
		break;
		
	// Keyboard messages
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		HandleKeyDown(wParam, lParam);
		// Handle special case for escape
		if (wParam == VK_ESCAPE) {
			::PostQuitMessage(0);
		}
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		HandleKeyUp(wParam, lParam);
		break;
	case WM_CHAR:
	case WM_SYSCHAR:
		HandleChar(wParam, lParam);
		break;
		
	case WM_PAINT:
		break;
	case WM_SIZE:
	{
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);

		//llri::swapchain::resize(width, height);
	}
	break;
	case WM_DESTROY:
		printf("WM_DESTROY received\n");
		gMainWindow = nullptr;
		mercury::Application::GetCurrentApplication()->OnClose();
		::PostQuitMessage(0);
		break;
	default:
		return ::DefWindowProcW(hwnd, message, wParam, lParam);
	}

	return 0;
}

namespace mercury::ll::os
{
  const OSInfo &OS::GetInfo() { 
    static OSInfo info{OSType::Windows, OSArchitecture::x64};
    return info; 
  }

  void OS::Initialize() { 
    std::cout << "OS::Initialize (Emscripten)" << std::endl;
  }

  void OS::Shutdown() {
    std::cout << "OS::Shutdown (Emscripten)" << std::endl;
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
    gWinSystemInstance = GetModuleHandleA(nullptr);
    auto& config = Application::GetCurrentApplication()->GetConfig();
    auto& winCfg = config.window;

    bool is_maximized = desc.maximized;

    WNDCLASSEXW windowClass = {};

    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = &WndProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = gWinSystemInstance;
    windowClass.hIcon = ::LoadIcon(gWinSystemInstance, IDI_APPLICATION);
    windowClass.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    windowClass.lpszMenuName = nullptr;
    windowClass.lpszClassName = winClassName;
    windowClass.hIconSm = ::LoadIcon(gWinSystemInstance, IDI_APPLICATION);

    static ATOM atom = ::RegisterClassExW(&windowClass);

    int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect = { 0, 0, static_cast<LONG>(winCfg.width), static_cast<LONG>(winCfg.height) };
    ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    // Center the window within the screen. Clamp to 0, 0 for the top-left corner.
    int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
    int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

    c16 winTitleBuff[256] = { 0 };
    utils::string::utf8_to_utf16(desc.title.c_str(), winTitleBuff,256);

    DWORD exFlags = 0;
    DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;


    if (winCfg.fullscreen)
    {
      //exFlags |= WS_EX_TOPMOST;
      dwStyle = WS_VISIBLE | WS_POPUP;

      windowX = 0;
      windowY = 0;
      windowWidth = screenWidth - 0;
      windowHeight = screenHeight - 0;
    }

    HWND hWnd = ::CreateWindowExW(
      exFlags,
      winClassName,
      (const wchar_t*)winTitleBuff,
      dwStyle,
      windowX,
      windowY,
      windowWidth,
      windowHeight,
      nullptr,
      nullptr,
      gWinSystemInstance,
      nullptr
    );

    HMONITOR hMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFOEX monitorInfo = {};
    monitorInfo.cbSize = sizeof(MONITORINFOEX);
    ::GetMonitorInfo(hMonitor, &monitorInfo);

    if (is_maximized)
    {
      ::ShowWindow(hWnd, SW_SHOWMAXIMIZED);
    }
    else
    {
      ::ShowWindow(hWnd, SW_SHOW);
    }

    ::UpdateWindow(hWnd);


    gMainWindow = hWnd;
  }

  void OS::DestroyNativeWindow()
  {

  }

  void OS::SetNativeWindowTitle(const std::u8string &title)
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
    MSG msg = {};
    for (int i = 0; i < 4; ++i)
    {
      if (::PeekMessage(&msg, gMainWindow, 0, 0, PM_REMOVE))
      {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
      }
    }
  }

  void OS::FatalFail(const char* reason)
  {
    MLOG_FATAL(u8"Fatal error: %s", reason);
    DebugBreak();
  }


  void* OS::LoadSharedLibrary(const char8_t* utf8libname)
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

	std::u8string OS::GetSharedLibraryFullFilename(void* libHandle)
  {
    if (libHandle == nullptr)
    {
      return std::u8string();
    }

    HMODULE hModule = static_cast<HMODULE>(libHandle);
    wchar_t filename[MAX_PATH] = { 0 };
    GetModuleFileNameW(hModule, filename, MAX_PATH);

    c8 filenameU8[1024] = { 0 };
    utils::string::utf16_to_utf8((const char16_t*)filename, filenameU8, 1024);
    return std::u8string(filenameU8);
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
      if (gMainWindow == nullptr)
      {
          widthOut = 0;
          heightOut = 0;
          return;
      }
      RECT rect;
      if (GetClientRect(gMainWindow, &rect))
      {
          widthOut = rect.right - rect.left;
          heightOut = rect.bottom - rect.top;
      }
      else
      {
          widthOut = 0;
          heightOut = 0;
	  }
  }
  void OS::ImguiInitialize()
  {
      ImGui_ImplWin32_Init(gMainWindow);
  }
  void OS::ImguiNewFrame()
  {
      ImGui_ImplWin32_NewFrame();
  }
  void OS::ImguiShutdown()
  {
      ImGui_ImplWin32_Shutdown();
  }

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

  OS* gOS = nullptr;
} 

// entry point for win32

void WinPlatformInit() {
  timeBeginPeriod(1);
  SetProcessDPIAware();

  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
  setlocale(LC_CTYPE, "en_US.UTF-8");
  setlocale(LC_COLLATE, "en_US.UTF-8");

  GetSystemInfo(&gSysInfo);
}

int main(int argc, char **argv) {
  // Set console code page to UTF-8
  WinPlatformInit();

  std::cout << "WIN32 start console" << std::endl;

  RunCurrentApplication();

  return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  WinPlatformInit();
  std::cout << "WIN32 start WinMain" << std::endl;

  RunCurrentApplication();

  return 0;
}




#endif