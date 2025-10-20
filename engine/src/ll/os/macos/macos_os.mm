#include "ll/os.h"
#ifdef MERCURY_LL_OS_MACOS

#include "application.h"
#include "mercury_log.h"
#include "mercury_utils.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/mman.h>
#include <mach/mach.h>
#include <mach/vm_map.h>
#include <algorithm>

// macOS specific includes
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include "ll/graphics/vulkan/vk_swapchain.h"
#endif

#ifdef MERCURY_LL_GRAPHICS_WEBGPU
#include "ll/graphics/webgpu/webgpu_graphics.h"
#endif

#include "../../../imgui/imgui_impl.h"

// Global state
NSWindow* gMainWindow = nullptr;
NSView* gMainView = nullptr;
CAMetalLayer* gMetalLayer = nullptr;
bool gWindowReady = false;
bool gApplicationRunning = true;

// Forward declarations
@interface MercuryAppDelegate : NSObject <NSApplicationDelegate>
@end

@interface MercuryWindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation MercuryAppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    MLOG_DEBUG(u8"Application did finish launching");
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    MLOG_DEBUG(u8"Application will terminate");
    gApplicationRunning = false;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}
@end

@implementation MercuryWindowDelegate
- (void)windowDidResize:(NSNotification *)notification {
    MLOG_DEBUG(u8"Window did resize");
    // Notify graphics layer about the resize so the swapchain drawable size
    // and ImGui display size can be updated accordingly.
    if (mercury::ll::graphics::gSwapchain) {
        unsigned int w = mercury::ll::graphics::gSwapchain->GetWidth();
        unsigned int h = mercury::ll::graphics::gSwapchain->GetHeight();
        // Query actual view size and pass to swapchain resize
        if (gMainView) {
            NSRect bounds = gMainView.bounds;
            w = static_cast<unsigned int>(bounds.size.width);
            h = static_cast<unsigned int>(bounds.size.height);
        }
        // Swapchain::Resize expects u16 values. Clamp to a safe range.
        mercury::u16 rw = (mercury::u16)std::min<unsigned int>(w, 0xFFFFu);
        mercury::u16 rh = (mercury::u16)std::min<unsigned int>(h, 0xFFFFu);
        mercury::ll::graphics::gSwapchain->Resize(rw, rh);
    }
}

- (void)windowDidMove:(NSNotification *)notification {
    MLOG_DEBUG(u8"Window did move");
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
    MLOG_DEBUG(u8"Window did become key");
}

- (void)windowDidResignKey:(NSNotification *)notification {
    MLOG_DEBUG(u8"Window did resign key");
}

- (void)windowWillClose:(NSNotification *)notification {
    MLOG_DEBUG(u8"Window will close");
    gMainView = nullptr;
    gMetalLayer = nullptr;
    gWindowReady = false;
    mercury::Application::GetCurrentApplication()->OnClose();
}
@end

namespace mercury::ll::os
{
    const OSInfo& OS::GetInfo() {
        static OSInfo info{OSType::MacOS, OSArchitecture::ARM64};
        return info;
    }

    void OS::Initialize() {
        MLOG_DEBUG(u8"OS::Initialize (macOS)");
        
        // Initialize NSApplication
        [NSApplication sharedApplication];
        
        // Set up app delegate
        MercuryAppDelegate* appDelegate = [[MercuryAppDelegate alloc] init];
        [NSApp setDelegate:appDelegate];
        
        // Set activation policy to allow the app to appear in dock
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    }

    void OS::Shutdown() {
        MLOG_DEBUG(u8"OS::Shutdown (macOS)");
        gApplicationRunning = false;
    }

    bool OS::IsFocused() {
        return gMainWindow && [gMainWindow isKeyWindow];
    }

    void OS::Sleep(u32 milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    void* OS::GetCurrentNativeWindowHandle() {
        return (__bridge void*)gMainView;
    }

    void OS::CreateNativeWindow(NativeWindowDescriptor& desc) {
        MLOG_DEBUG(u8"Creating macOS native window");
        
        auto& config = Application::GetCurrentApplication()->GetConfig();
        auto& winCfg = config.window;
        
        // Create the main window
        NSRect windowRect = NSMakeRect(100, 100, desc.width, desc.height);
        NSUInteger windowStyle = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | 
                                NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
        
        if (desc.fullscreen) {
            windowStyle = NSWindowStyleMaskBorderless;
            NSRect screenRect = [[NSScreen mainScreen] frame];
            windowRect = screenRect;
        }
        
        gMainWindow = [[NSWindow alloc] initWithContentRect:windowRect
                                                  styleMask:windowStyle
                                                    backing:NSBackingStoreBuffered
                                                      defer:NO];
        
        if (!gMainWindow) {
            MLOG_ERROR(u8"Failed to create NSWindow");
            return;
        }
        
        // Set window properties
        [gMainWindow setTitle:[NSString stringWithUTF8String:reinterpret_cast<const char*>(desc.title.c_str())]];
        [gMainWindow setDelegate:[[MercuryWindowDelegate alloc] init]];
        
        // Create the main view
        gMainView = [[NSView alloc] initWithFrame:windowRect];
        [gMainWindow setContentView:gMainView];
        
        // Create Metal layer for future graphics support
        gMetalLayer = [CAMetalLayer layer];
        if (gMetalLayer) {
            gMetalLayer.frame = gMainView.bounds;
            gMetalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
            gMetalLayer.contentsScale = [[NSScreen mainScreen] backingScaleFactor];
            [gMainView setLayer:gMetalLayer];
            [gMainView setWantsLayer:YES];
            MLOG_DEBUG(u8"Metal layer created successfully");
        } else {
            MLOG_WARNING(u8"Failed to create Metal layer");
        }
        
        // Show the window
        [gMainWindow makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
        
        gWindowReady = true;
        MLOG_DEBUG(u8"macOS window created successfully");
    }

    void OS::DestroyNativeWindow() {
        MLOG_DEBUG(u8"Destroying macOS native window");
        if (gMainWindow) {
            [gMainWindow close];
            gMainWindow = nullptr;
            gMainView = nullptr;
            gMetalLayer = nullptr;
            gWindowReady = false;
        }
    }

    void OS::SetNativeWindowTitle(const c8string& title) {
        if (gMainWindow) {
            [gMainWindow setTitle:[NSString stringWithUTF8String:reinterpret_cast<const char*>(title.c_str())]];
        }
    }

    void OS::SetNativeWindowSize(int width, int height) {
        if (gMainWindow) {
            NSRect frame = gMainWindow.frame;
            frame.size.width = width;
            frame.size.height = height;
            [gMainWindow setFrame:frame display:YES];
        }
    }

    void OS::SetNativeWindowFullscreen(bool fullscreen) {
        if (gMainWindow) {
            if (fullscreen) {
                [gMainWindow toggleFullScreen:nil];
            } else {
                [gMainWindow toggleFullScreen:nil];
            }
        }
    }

    bool OS::IsNativeWindowFullscreen() {
        if (gMainWindow) {
            return ([gMainWindow styleMask] & NSWindowStyleMaskFullScreen) != 0;
        }
        return false;
    }

    void OS::Update() {
        // Process events
        NSEvent* event;
        do {
            event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                      untilDate:[NSDate distantPast]
                                         inMode:NSDefaultRunLoopMode
                                        dequeue:YES];
            if (event) {
                [NSApp sendEvent:event];
            }
        } while (event);
        
        // Call the application tick directly for Metal devices to avoid threading issues
        // This ensures graphics operations happen on the main thread
        TickCurrentApplication();
    }

    void OS::FatalFail(const char* reason) {
        MLOG_FATAL(u8"Fatal error: %s", reason);
        exit(EXIT_FAILURE);
    }

    void* OS::LoadSharedLibrary(const c8* utf8libname) {
        const char* libname = reinterpret_cast<const char*>(utf8libname);
        void* handle = dlopen(libname, RTLD_LAZY);
        
        if (!handle) {
            MLOG_ERROR(u8"Failed to load shared library: %s, error: %s", 
                      utf8libname, dlerror());
            return nullptr;
        }
        
        return handle;
    }

    c8string OS::GetSharedLibraryFullFilename(void* libHandle) {
        if (!libHandle) {
            return c8string();
        }
        
        Dl_info info;
        if (dladdr(libHandle, &info)) {
            return c8string(reinterpret_cast<const c8*>(info.dli_fname));
        }
        
        MLOG_ERROR(u8"Failed to get library filename");
        return c8string();
    }

    bool OS::UnloadSharedLibrary(void* library) {
        return dlclose(library) == 0;
    }

    void* OS::GetFuncPtrImpl(void* library, const char* funcName) {
        return dlsym(library, funcName);
    }

    size_t OS::GetPageSize() {
        return getpagesize();
    }

    void* OS::ReserveMemory(size_t size) {
        vm_address_t address = 0;
        kern_return_t result = vm_allocate(mach_task_self(), &address, size, VM_FLAGS_ANYWHERE);
        if (result != KERN_SUCCESS) {
            MLOG_ERROR(u8"Failed to reserve memory: %zu bytes", size);
            return nullptr;
        }
        return (void*)address;
    }

    void OS::CommitMemory(void* ptr, size_t size) {
        // On macOS, vm_allocate already commits the memory
        // This is a no-op for compatibility
    }

    void OS::DecommitMemory(void* ptr, size_t size) {
        // On macOS, we can decommit memory using vm_deallocate
        vm_deallocate(mach_task_self(), (vm_address_t)ptr, size);
    }

    void OS::ReleaseMemory(void* ptr, size_t size) {
        vm_deallocate(mach_task_self(), (vm_address_t)ptr, size);
    }

    const char* OS::GetName() {
        static const char* osName = "macOS";
        return osName;
    }

    bool OS::IsNativeWindowReady() {
        return gWindowReady && gMainWindow != nullptr;
    }

    void OS::GetActualWindowSize(unsigned int& widthOut, unsigned int& heightOut) {
        if (gMainView) {
            NSRect frame = gMainView.frame;
            widthOut = static_cast<unsigned int>(frame.size.width);
            heightOut = static_cast<unsigned int>(frame.size.height);
        } else {
            widthOut = 0;
            heightOut = 0;
        }
    }

// IMGUI_IMPL_API bool     ImGui_ImplOSX_Init(NSView* _Nonnull view);
// IMGUI_IMPL_API void     ImGui_ImplOSX_Shutdown();
// IMGUI_IMPL_API void     ImGui_ImplOSX_NewFrame(NSView* _Nullable view);
    void OS::ImguiInitialize() {
        // ImGui macOS implementation would go here
        // For now, this is a placeholder
        MLOG_DEBUG(u8"ImGui macOS initialization placeholder");
        ImGui_ImplOSX_Init(gMainView);
        // Note: For Metal, ImGui initialization is handled in the graphics layer
    }

    void OS::ImguiNewFrame() {
        // ImGui macOS new frame would go here
        // For now, this is a placeholder
        // Note: For Metal, ImGui new frame is handled in the graphics layer
        ImGui_ImplOSX_NewFrame(gMainView);

    }

    void OS::ImguiShutdown() {
        // ImGui macOS shutdown would go here
        // For now, this is a placeholder
        MLOG_DEBUG(u8"ImGui macOS shutdown placeholder");
        ImGui_ImplOSX_Shutdown();
        // Note: For Metal, ImGui shutdown is handled in the graphics layer
    }

#ifdef MERCURY_LL_GRAPHICS_VULKAN
    void* OS::CreateVkSurface(void* vk_instance, void* allocations_callback) {
        // Vulkan surface creation for macOS would go here
        // This requires MoltenVK or similar Vulkan implementation
        MLOG_DEBUG(u8"Vulkan surface creation not implemented for macOS yet");
        return nullptr;
    }

    bool OS::IsQueueSupportPresent(void* vk_physical_device, u32 queueIndex) {
        // Check if the queue supports presentation on macOS
        return false; // Placeholder
    }
#endif

#ifdef MERCURY_LL_GRAPHICS_WEBGPU
    wgpu::Surface* gWebGPUSurfacePtr = nullptr;
    
    void* OS::GetWebGPUNativeWindowHandle() {
        if (!gMainView) {
            MLOG_ERROR(u8"No main view available for WebGPU surface");
            return nullptr;
        }
        
        // Create WebGPU surface descriptor for macOS
        wgpu::SurfaceDescriptorFromMetalLayer metalDesc = {};
        metalDesc.sType = wgpu::SType::SurfaceSourceMetalLayer;
        metalDesc.layer = (__bridge void*)gMetalLayer;
        
        wgpu::SurfaceDescriptor surfaceDesc = {};
        surfaceDesc.nextInChain = &metalDesc;
        surfaceDesc.label = "Main Window WebGPU Surface";
        
        // Create the surface using the global WebGPU instance
        wgpu::Surface surface = wgpuInstance.CreateSurface(&surfaceDesc);
        
        if (!surface) {
            MLOG_ERROR(u8"Failed to create WebGPU surface");
            return nullptr;
        }
        
        MLOG_DEBUG(u8"WebGPU macOS surface created successfully");
        
        // Allocate and return the surface pointer
        gWebGPUSurfacePtr = new wgpu::Surface(surface);
        return gWebGPUSurfacePtr;
    }
    
    void OS::WebGPUPresent() {
        if (gWebGPUSurfacePtr) {
            gWebGPUSurfacePtr->Present();
        }
    }
#endif

    OS* gOS = nullptr;
}

// Entry point for macOS
int main(int argc, char** argv) {
    // Set up locale for UTF-8 support
    setlocale(LC_ALL, "en_US.UTF-8");
    
    MLOG_DEBUG(u8"macOS application starting");
    
    // For command-line tools or simple apps, use direct approach
    // For GUI apps, NSApplicationMain will be called automatically
    // when the app is launched from a .app bundle
    
    // Initialize and run the application
    RunCurrentApplication();
    
    return 0;
}

// Alternative entry point for NSApplication-based apps
// This is called when the app is launched from a .app bundle
extern "C" int NSApplicationMain(int argc, const char* argv[]) {
    // Set up locale for UTF-8 support
    setlocale(LC_ALL, "en_US.UTF-8");
    
    MLOG_DEBUG(u8"macOS NSApplication starting");
    
    // Initialize and run the application
    RunCurrentApplication();
    
    return 0;
}

#endif