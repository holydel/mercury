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
void* gMainWindow = nullptr;
SYSTEM_INFO gSysInfo;
constexpr const wchar_t* winClassName = L"MercuryWindow";

#include <ppltasks.h>

unsigned int gCurrentWindowWidth = 1280;
unsigned int gCurrentWindowHeight = 720;

using namespace concurrency;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

using Microsoft::WRL::ComPtr;

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
          widthOut = gCurrentWindowWidth;
          heightOut = gCurrentWindowHeight;
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

// entry point for UWP


// Main entry point for our app. Connects the app with the Windows shell and handles application lifecycle events.
ref class MercuryUWPApp sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
public:
    MercuryUWPApp();

    // IFrameworkView methods.
    virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
    virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
    virtual void Load(Platform::String^ entryPoint);
    virtual void Run();
    virtual void Uninitialize();

protected:
    // Application lifecycle event handlers.
    void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
    void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
    void OnResuming(Platform::Object^ sender, Platform::Object^ args);

    // Window event handlers.
    void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
    void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
    void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);

    // DisplayInformation event handlers.
    void OnDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
    void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
    void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);

private:
    bool m_windowClosed;
    bool m_windowVisible;
};


ref class Direct3DApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView()
    {
        return ref new MercuryUWPApp();
    }
};

MercuryUWPApp::MercuryUWPApp() :
	m_windowClosed(false),
	m_windowVisible(true)
{
}


// The first method called when the IFrameworkView is being created.
void MercuryUWPApp::Initialize(CoreApplicationView^ applicationView)
{
	// Register event handlers for app lifecycle. This example includes Activated, so that we
	// can make the CoreWindow active and start rendering on the window.
	applicationView->Activated +=
		ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &MercuryUWPApp::OnActivated);

	CoreApplication::Suspending +=
		ref new EventHandler<SuspendingEventArgs^>(this, &MercuryUWPApp::OnSuspending);

	CoreApplication::Resuming +=
		ref new EventHandler<Platform::Object^>(this, &MercuryUWPApp::OnResuming);
}

// Called when the CoreWindow object is created (or re-created).
void MercuryUWPApp::SetWindow(CoreWindow^ window)
{
	window->SizeChanged +=
		ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &MercuryUWPApp::OnWindowSizeChanged);

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &MercuryUWPApp::OnVisibilityChanged);

	window->Closed +=
		ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &MercuryUWPApp::OnWindowClosed);

	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	currentDisplayInformation->DpiChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &MercuryUWPApp::OnDpiChanged);

	currentDisplayInformation->OrientationChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &MercuryUWPApp::OnOrientationChanged);

	DisplayInformation::DisplayContentsInvalidated +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &MercuryUWPApp::OnDisplayContentsInvalidated);

    Windows::UI::Core::SystemNavigationManager::GetForCurrentView()->BackRequested +=
        ref new EventHandler<Windows::UI::Core::BackRequestedEventArgs^>(
            [this](Platform::Object^ sender, Windows::UI::Core::BackRequestedEventArgs^ args)
            {
                args->Handled = true;
                m_windowClosed = true;

                // Clean shutdown
                CoreApplication::Exit();
            });

    gMainWindow = reinterpret_cast<IUnknown*>(window);

	gCurrentWindowHeight = window->Bounds.Height;
	gCurrentWindowWidth = window->Bounds.Width;
    InitializeCurrentApplication();
}

// Initializes scene resources, or loads a previously saved app state.
void MercuryUWPApp::Load(Platform::String^ entryPoint)
{

}

// This method is called after the window becomes active.
void MercuryUWPApp::Run()
{
    while (!m_windowClosed)
    {       
        if (m_windowVisible)
        {
            CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
            
            // Tick the application
            TickCurrentApplication();
        }
        else
        {
            CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
            
            // Still tick even when not visible (but at lower frequency)
            ::Sleep(16); // ~60 FPS when hidden
            TickCurrentApplication();
        }
        
        // Check if window was closed during tick
        if (m_windowClosed)
        {
            break;
        }
    }
    
    // Ensure proper shutdown when loop exits
    MLOG_INFO("Application run loop exiting, performing cleanup...");
}

// Required for IFrameworkView.
// Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
// class is torn down while the app is in the foreground.
void MercuryUWPApp::Uninitialize()
{
    ShutdownCurrentApplication();
}

// Application lifecycle event handlers.

void MercuryUWPApp::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	// Run() won't start until the CoreWindow is activated.
	CoreWindow::GetForCurrentThread()->Activate();
}

void MercuryUWPApp::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
	// Save app state asynchronously after requesting a deferral. Holding a deferral
	// indicates that the application is busy performing suspending operations. Be
	// aware that a deferral may not be held indefinitely. After about five seconds,
	// the app will be forced to exit.
	SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

	//create_task([this, deferral]()
	//	{
	//		m_main->OnSuspending();
	//		deferral->Complete();
	//	});
}

void MercuryUWPApp::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.

	//m_main->OnResuming();
}

// Window event handlers.

void MercuryUWPApp::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	gCurrentWindowWidth = sender->Bounds.Width;
	gCurrentWindowHeight = sender->Bounds.Height;

	//GetDeviceResources()->SetLogicalSize(Size(sender->Bounds.Width, sender->Bounds.Height));
	//m_main->OnWindowSizeChanged();
}

void MercuryUWPApp::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
}

void MercuryUWPApp::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	m_windowClosed = true;
}

// DisplayInformation event handlers.

void MercuryUWPApp::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
	// Note: The value for LogicalDpi retrieved here may not match the effective DPI of the app
	// if it is being scaled for high resolution devices. Once the DPI is set on DeviceResources,
	// you should always retrieve it using the GetDpi method.
	// See DeviceResources.cpp for more details.
	//GetDeviceResources()->SetDpi(sender->LogicalDpi);
	//m_main->OnWindowSizeChanged();
}

void MercuryUWPApp::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	//GetDeviceResources()->SetCurrentOrientation(sender->CurrentOrientation);
	//m_main->OnWindowSizeChanged();
}

void MercuryUWPApp::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	//GetDeviceResources()->ValidateDevice();
}

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
    auto direct3DApplicationSource = ref new Direct3DApplicationSource();
    CoreApplication::Run(direct3DApplicationSource);

	return 0;
}

#endif