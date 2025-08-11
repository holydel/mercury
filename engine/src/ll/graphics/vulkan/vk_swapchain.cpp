#include "vk_graphics.h"

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include "mercury_log.h"
#include "mercury_application.h"
#include "../ll_graphics.h"
#include "vk_swapchain.h"
using namespace mercury;
using namespace mercury::ll::graphics;

struct SwapchainExt
{
	//instance extensions
	bool hasSurfaceCapabilities2 : 1 = false;
	bool hasSurfaceMaintenance1 : 1 = false;
	bool hasSwapchainColorspace : 1 = false;
	bool hasDisplay : 1 = false;
	bool hasDisplay2Props : 1 = false;
	bool hasDirectModeDisplay : 1 = false;
	bool hasProtectedCapabilities : 1 = false;
	//device extensions
	bool hasMaintenance1 : 1 = false;
	bool hasPresentId : 1 = false;
	bool hasPresentWait : 1 = false;
	bool hasGoogleTiming : 1 = false;
	bool hasExclusiveFullscreen : 1 = false;
	bool hasHDRMetadata : 1 = false;
	bool hasNVAcquireDisplay : 1 = false;
} gVkSwapchainEXT;

void vkSwapchainRequestInstanceExtensions(VKInstanceExtender& instanceExtender)
{
#ifdef MERCURY_LL_OS_WIN32
	instanceExtender.TryAddExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#ifdef MERCURY_LL_OS_ANDROID
	instanceExtender.TryAddExtension(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif
#ifdef MERCURY_LL_OS_LINUX
	//VK_KHR_xcb_surface
	instanceExtender.TryAddExtension("VK_KHR_xcb_surface");
#endif
#ifdef MERCURY_LL_OS_MACOS
	instanceExtender.TryAddExtension(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#endif

	gVkSwapchainEXT.hasProtectedCapabilities = instanceExtender.TryAddExtension(VK_KHR_SURFACE_PROTECTED_CAPABILITIES_EXTENSION_NAME);

	gVkSwapchainEXT.hasSurfaceCapabilities2 = instanceExtender.TryAddExtension(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
	gVkSwapchainEXT.hasSwapchainColorspace = instanceExtender.TryAddExtension(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);

	if (gSwapchainConfig.useDisplay)
	{
		gVkSwapchainEXT.hasDisplay = instanceExtender.TryAddExtension(VK_KHR_DISPLAY_EXTENSION_NAME);
		gVkSwapchainEXT.hasDisplay2Props = instanceExtender.TryAddExtension(VK_KHR_GET_DISPLAY_PROPERTIES_2_EXTENSION_NAME);
		gVkSwapchainEXT.hasDirectModeDisplay = instanceExtender.TryAddExtension(VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME);
	}

	if (gVkSwapchainEXT.hasSurfaceCapabilities2)
	{
		gVkSwapchainEXT.hasSurfaceMaintenance1 = instanceExtender.TryAddExtension(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
	}
}

void vkSwapchainRequestDeviceExtensions(VKDeviceExtender& device_extender)
{
	device_extender.TryAddExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME); //TODO: fill device info with supported features

	if (gVkSwapchainEXT.hasSurfaceMaintenance1)
	{
		gVkSwapchainEXT.hasMaintenance1 = device_extender.TryAddExtension(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME);
	}

	gVkSwapchainEXT.hasPresentId = device_extender.TryAddExtension(VK_KHR_PRESENT_ID_EXTENSION_NAME);
	gVkSwapchainEXT.hasPresentWait = device_extender.TryAddExtension(VK_KHR_PRESENT_WAIT_EXTENSION_NAME);
	gVkSwapchainEXT.hasGoogleTiming = device_extender.TryAddExtension(VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME);
	gVkSwapchainEXT.hasHDRMetadata = device_extender.TryAddExtension(VK_EXT_HDR_METADATA_EXTENSION_NAME);

#ifdef MERCURY_LL_OS_WIN32
	if (gSwapchainConfig.needExclusiveFullscreen)
	{
		gVkSwapchainEXT.hasExclusiveFullscreen = device_extender.TryAddExtension(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
	}

	if (gSwapchainConfig.useDisplay)
	{
		gVkSwapchainEXT.hasNVAcquireDisplay = device_extender.TryAddExtension(VK_NV_ACQUIRE_WINRT_DISPLAY_EXTENSION_NAME);
	}
#endif
}

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