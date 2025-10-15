#include "ll/os.h"
#ifdef MERCURY_LL_OS_ANDROID

#include "application.h"
#include <android/log.h>
#include <android/native_activity.h>
#include <android/window.h>
#include <iostream>
#include "mercury_utils.h"
#include "mercury_log.h"
#include <android/native_window.h>
#include <android/input.h>

#include <android/ndk-version.h>

#include <string>
#include <thread>
#include <atomic>

#include <dlfcn.h>
#include <unordered_map>

#include <fcntl.h>      // For open, O_RDONLY, O_RDWR, O_CREAT, etc.
#include <sys/mman.h>   // For mmap, munmap
#include <sys/stat.h>   // For fstat
#include <link.h> 
#include <unistd.h> // Required for usleep()

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include "ll/graphics/vulkan/vk_swapchain.h"
#endif

#ifdef MERCURY_LL_GRAPHICS_WEBGPU
#include "ll/graphics/webgpu/webgpu_graphics.h"
#endif

#include "../../../imgui/imgui_impl.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <game-activity/GameActivity.h>

ANativeWindow* gMainWindow = nullptr;
ANativeActivity* gMainActivity = nullptr;
extern int32_t ImGui_ImplAndroid_HandleInputEvent(const AInputEvent* input_event);
static AInputQueue* gMainInputQueue = nullptr;

namespace mercury::ll::os
{
    const OSInfo& OS::GetInfo() {
        static OSInfo info{ OSType::Android, OSArchitecture::x64 };
        return info;
    }

    void OS::Initialize() {
		MLOG_DEBUG(u8"OS::Initialize (Android)");
    }

    void OS::Shutdown() {
        MLOG_DEBUG(u8"OS::Shutdown (Android)");
    }

    bool OS::IsFocused() { return true; }

    void OS::Sleep(u32 milliseconds) {
        ::usleep(milliseconds * 1000);
    }

    void* OS::GetCurrentNativeWindowHandle() {
        return gMainWindow;
    }

    void OS::CreateNativeWindow(NativeWindowDescriptor& desc)
    {
		//Android window is created by the system and we just get the handle to it from the activity
    }

    void OS::DestroyNativeWindow()
    {

    }

    void OS::SetNativeWindowTitle(const std::u8string& title)
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
       // DebugBreak();
    }


    void* OS::LoadSharedLibrary(const char8_t* utf8libname)
    {
        return dlopen((const char*)utf8libname, RTLD_NOW);
    }


    std::u8string OS::GetSharedLibraryFullFilename(void* handle)
    {
        if (!handle) return u8"";

        uintptr_t base = reinterpret_cast<uintptr_t>(handle);
        char maps_path[64];  // Sufficient for /proc/%d/maps
        snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", getpid());

        FILE* fp = fopen(maps_path, "r");
        if (!fp) {
            std::cerr << "Failed to open " << maps_path << std::endl;
            return u8"";
        }

        char line[BUFSIZ];
        std::u8string lib_path;
        while (fgets(line, sizeof(line), fp) != NULL) {
            char* endptr;
            uintptr_t start = strtoul(line, &endptr, 16);
            if (start == base && endptr && *endptr == '-') {  // Valid start address
                line[strlen(line) - 1] = '\0';  // Chomp newline
                char* path_start = strchr(line, '/');
                if (path_start) {
                    lib_path = reinterpret_cast<const char8_t*>(path_start);
                    break;
                }
            }
        }
        fclose(fp);

        return lib_path;
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
        return getpagesize();;
    }

    void* OS::ReserveMemory(size_t size)
    {
        void* ptr = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED) {
            return nullptr;
        }
        return ptr;
    }

    void OS::CommitMemory(void* ptr, size_t size)
    {
        mprotect(ptr, size, PROT_READ | PROT_WRITE);
    }

    void OS::DecommitMemory(void* ptr, size_t size)
    {
        mprotect(ptr, size, PROT_NONE);
    }

    void OS::ReleaseMemory(void* ptr, size_t size)
    {
        munmap(ptr, size);
    }

    const char* OS::GetName()
    {
        static const char* osName = "Android";
        return osName;
    }

    bool OS::IsNativeWindowReady()
    {
        return gMainWindow != nullptr;
    }

    void OS::GetActualWindowSize(unsigned int& widthOut, unsigned int& heightOut)
    {
        widthOut = 1920;
		heightOut = 1080;
		//TODO get real size
    }
    void OS::ImguiInitialize()
    {
        ImGui_ImplAndroid_Init(gMainWindow);
    }
    void OS::ImguiNewFrame()
    {
        ImGui_ImplAndroid_NewFrame();
    }
    void OS::ImguiShutdown()
    {
        ImGui_ImplAndroid_Shutdown();
    }

#ifdef MERCURY_LL_GRAPHICS_VULKAN
    void* OS::CreateVkSurface(void* vk_instance, void* allocations_callback)
    {
        VkAndroidSurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
        createInfo.window = static_cast<ANativeWindow*>(gMainWindow);

        VK_CALL(vkCreateAndroidSurfaceKHR(gVKInstance, &createInfo, (VkAllocationCallbacks*)allocations_callback, &gVKSurface));
        return gVKSurface;
    }

    bool OS::IsQueueSupportPresent(void* vk_physical_device, u32 queueIndex)
    {
       // bool supportPresent = vkGetPhysicalDeviceAndroidPresentationSupportKHR != nullptr ? vkGetPhysicalDeviceAndroidPresentationSupportKHR((VkPhysicalDevice)vk_physical_device, queueIndex) : false;
		return true; //Android always supports presentation on any queue
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

void onStart(ANativeActivity* activity)
{
    MLOG_DEBUG(u8"onStart");
}

/**
 * NativeActivity has resumed.  See Java documentation for Activity.onResume()
 * for more information.
 */
void onResume(ANativeActivity* activity)
{
    MLOG_DEBUG(u8"onResume");
}

/**
 * Framework is asking NativeActivity to save its current instance state.
 * See Java documentation for Activity.onSaveInstanceState() for more
 * information.  The returned pointer needs to be created with malloc();
 * the framework will call free() on it for you.  You also must fill in
 * outSize with the number of bytes in the allocation.  Note that the
 * saved state will be persisted, so it can not contain any active
 * entities (pointers to memory, file descriptors, etc).
 */
void* onSaveInstanceState(ANativeActivity* activity, size_t* outSize)
{
    MLOG_DEBUG(u8"onSaveInstanceState");

    void* savedData = malloc(64);
    *outSize = 64;
    return savedData;
}

/**
 * NativeActivity has paused.  See Java documentation for Activity.onPause()
 * for more information.
 */
void onPause(ANativeActivity* activity)
{
    MLOG_DEBUG(u8"onPause");
}

/**
 * NativeActivity has stopped.  See Java documentation for Activity.onStop()
 * for more information.
 */
void onStop(ANativeActivity* activity)
{
    MLOG_DEBUG(u8"onStop");
}

/**
 * NativeActivity is being destroyed.  See Java documentation for Activity.onDestroy()
 * for more information.
 */
void onDestroy(ANativeActivity* activity)
{

    //need_to_stop = true;

    //while (is_running)
    //{
    //	std::this_thread::sleep_for(16ms);
    //}

    //mainThread->join();
    //delete mainThread;

    MLOG_DEBUG(u8"onDestroy");
}

/**
 * Focus has changed in this NativeActivity's window.  This is often used,
 * for example, to pause a game when it loses input focus.
 */
void onWindowFocusChanged(ANativeActivity* activity, int hasFocus)
{
    MLOG_DEBUG(u8"onWindowFocusChanged");
}

/**
 * The drawing window for this native activity has been created.  You
 * can use the given native window object to start drawing.
 */
void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window)
{
    MLOG_DEBUG(u8"onNativeWindowCreated");
    gMainWindow = window;
}

/**
 * The drawing window for this native activity has been resized.  You should
 * retrieve the new size from the window and ensure that your rendering in
 * it now matches.
 */
void onNativeWindowResized(ANativeActivity* activity, ANativeWindow* window)
{
    MLOG_DEBUG(u8"onNativeWindowResized");
}

/**
 * The drawing window for this native activity needs to be redrawn.  To avoid
 * transient artifacts during screen changes (such resizing after rotation),
 * applications should not return from this function until they have finished
 * drawing their window in its current state.
 */
void onNativeWindowRedrawNeeded(ANativeActivity* activity, ANativeWindow* window)
{
    MLOG_DEBUG(u8"onNativeWindowRedrawNeeded");
}

/**
 * The drawing window for this native activity is going to be destroyed.
 * You MUST ensure that you do not touch the window object after returning
 * from this function: in the common case of drawing to the window from
 * another thread, that means the implementation of this callback must
 * properly synchronize with the other thread to stop its drawing before
 * returning from here.
 */
void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window)
{
    MLOG_DEBUG(u8"onNativeWindowDestroyed");
    gMainWindow = nullptr;
}
/**
 * The input queue for this native activity's window has been created.
 * You can use the given input queue to start retrieving input events.
 */
void onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue)
{
    MLOG_DEBUG(u8"onInputQueueCreated");
    gMainInputQueue = queue;
}

/**
 * The input queue for this native activity's window is being destroyed.
 * You should no longer try to reference this object upon returning from this
 * function.
 */
void onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue)
{
    gMainInputQueue = nullptr;
    MLOG_DEBUG(u8"onInputQueueDestroyed");
}

/**
 * The rectangle in the window in which content should be placed has changed.
 */
void onContentRectChanged(ANativeActivity* activity, const ARect* rect)
{
    MLOG_DEBUG(u8"onContentRectChanged");
}

/**
 * The current device AConfiguration has changed.  The new configuration can
 * be retrieved from assetManager.
 */
void onConfigurationChanged(ANativeActivity* activity)
{
    MLOG_DEBUG(u8"onConfigurationChanged");
}

/**
 * The system is running low on memory.  Use this callback to release
 * resources you do not need, to help the system avoid killing more
 * important processes.
 */
void onLowMemory(ANativeActivity* activity)
{
    MLOG_DEBUG(u8"onLowMemory");

}

void handle_cmd(android_app *pApp, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            // A new window is created, associate a renderer with it. You may replace this with a
            // "game" class if that suits your needs. Remember to change all instances of userData
            // if you change the class here as a reinterpret_cast is dangerous this in the
            // android_main function and the APP_CMD_TERM_WINDOW handler case.
            gMainWindow = pApp->window;
            MLOG_INFO(u8"APP_CMD_INIT_WINDOW");
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being destroyed. Use this to clean up your userData to avoid leaking
            // resources.
            //
            // We have to check if userData is assigned just in case this comes in really quickly
            gMainWindow = nullptr;
            MLOG_INFO(u8"APP_CMD_TERM_WINDOW");
            break;
        default:
            break;
    }
}

bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent) {
    auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
    return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
            sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
}

std::thread *gMainThread = nullptr;

void MainThread()
{
    MLOG_INFO(u8"Android main thread started");

    InitializeCurrentApplication();

    while(mercury::Application::GetCurrentApplication()->IsRunning())
    {
        TickCurrentApplication();
    }

    ShutdownCurrentApplication();
}

extern "C" {
void android_main(struct android_app *pApp) {
   // sleep(5);
    MLOG_INFO(u8"Start from GameActivity");
    pApp->onAppCmd = handle_cmd;
    android_app_set_motion_event_filter(pApp, motion_event_filter_func);

    gMainThread = new std::thread(MainThread);

    do {
        // Process all pending events before running game logic.
        bool done = false;
        while (!done) {
            // 0 is non-blocking.
            int timeout = 0;
            int events;
            android_poll_source *pSource;
            int result = ALooper_pollOnce(timeout, nullptr, &events,
                                          reinterpret_cast<void**>(&pSource));
            switch (result) {
                case ALOOPER_POLL_TIMEOUT:
                    [[clang::fallthrough]];
                case ALOOPER_POLL_WAKE:
                    // No events occurred before the timeout or explicit wake. Stop checking for events.
                    done = true;
                    break;
                case ALOOPER_EVENT_ERROR:
                    MLOG_ERROR(u8"ALooper_pollOnce returned an error");
                    break;
                case ALOOPER_POLL_CALLBACK:
                    break;
                default:
                    if (pSource) {
                        pSource->process(pApp, pSource);
                    }
            }
        }

        // Check if any user data is associated. This is assigned in handle_cmd
        if (pApp->userData) {
            //update engine
        }
    } while (!pApp->destroyRequested);

    if (gMainThread)
        gMainThread->join();
    delete gMainThread;
    gMainThread = nullptr;
}
}

extern "C" JNIEXPORT void ANativeActivity_onCreate(ANativeActivity* activity,
    void* savedState, size_t savedStateSize)
{
    gMainActivity = activity;


    activity->callbacks->onConfigurationChanged = onConfigurationChanged;
    activity->callbacks->onContentRectChanged = onContentRectChanged;
    activity->callbacks->onDestroy = onDestroy;
    activity->callbacks->onInputQueueCreated = onInputQueueCreated;
    activity->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;
    activity->callbacks->onLowMemory = onLowMemory;
    activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
    activity->callbacks->onNativeWindowRedrawNeeded = onNativeWindowRedrawNeeded;
    activity->callbacks->onNativeWindowResized = onNativeWindowResized;
    activity->callbacks->onPause = onPause;
    activity->callbacks->onResume = onResume;
    activity->callbacks->onSaveInstanceState = onSaveInstanceState;
    activity->callbacks->onStart = onStart;
    activity->callbacks->onStop = onStop;
    activity->callbacks->onWindowFocusChanged = onWindowFocusChanged;

    MLOG_INFO(u8"ANativeActivity_onCreate");
    //mercury::write_log_message("ANativeActivity_onCreate");
}

#endif