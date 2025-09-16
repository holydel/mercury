#pragma once

#include "mercury_api.h"

#ifdef MERCURY_LL_OS_LINUX
#include "ll/os.h"
#include "ll/graphics/vulkan/mercury_vulkan.h"

class LinuxDisplayServer
{
    public:
    LinuxDisplayServer() = default;
    virtual ~LinuxDisplayServer() = default;
    static LinuxDisplayServer* CreateWayland();

    virtual bool Initialize() = 0;
    virtual void CreateWindow(const mercury::ll::os::OS::NativeWindowDescriptor &desc)=0;
    virtual void DestroyWindow() = 0;
    virtual bool ProcessEvents() = 0;
    virtual void* GetWindowNativeHandle() = 0;
    virtual VkSurfaceKHR CreateSurface(void* vk_instance, void* allocations_callback) = 0;
    virtual bool CheckIfPresentSupportedOnQueue(void* vk_physical_device, mercury::u32 queueIndex) = 0;
    virtual bool IsNativeWindowReady() = 0; 
    virtual void GetActualWindowSize(unsigned int& widthOut, unsigned int& heightOut) = 0;

    virtual void ImguiInitialize() {}
    virtual void ImguiNewFrame() {}
    virtual void ImguiShutdown() {}
};

#endif