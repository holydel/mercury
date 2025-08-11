#include "vk_graphics.h"

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include "../mercury_swapchain.h"
#include "mercury_log.h"
#include "mercury_application.h"
#include "../ll_graphics.h"

void vkSwapchainRequestInstanceExtensions(VKInstanceExtender& instance_extender);
void vkSwapchainRequestDeviceExtensions(VKDeviceExtender& device_extender);

#endif