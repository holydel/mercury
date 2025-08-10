#include "vk_graphics.h"

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include "mercury_log.h"
#include "mercury_application.h"
#include "../ll_graphics.h"
using namespace mercury;
using namespace mercury::ll::graphics;

void *Swapchain::GetNativeHandle()
{
    return nullptr;
}

void Swapchain::Initialize(void* native_window_handle)
{
    MLOG_DEBUG(u8"Initialize Swapchain (Vulkan)");
}

void Swapchain::Shutdown()
{
    MLOG_DEBUG(u8"Shutdown Swapchain (Vulkan)");
}

void Swapchain::AcquireNextImage()
{
}

void Swapchain::Present()
{
}

void Swapchain::SetFullscreen(bool fullscreen)
{
}

#endif