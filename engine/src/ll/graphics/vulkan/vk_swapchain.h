#include "vk_graphics.h"

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include "../mercury_swapchain.h"
#include "mercury_log.h"
#include "mercury_application.h"
#include "../ll_graphics.h"

void vkSwapchainRequestInstanceExtensions(VKInstanceExtender& instance_extender);
void vkSwapchainRequestDeviceExtensions(VKDeviceExtender& device_extender);

extern VkSwapchainKHR gVKSwapchain;
extern VkSwapchainKHR gVKOldSwapchain;
extern VkSurfaceKHR gVKSurface;
extern VkSurfaceCapabilitiesKHR gVKSurfaceCaps;
extern VkFormat gVKSurfaceFormat;
extern VkSampleCountFlagBits gVKSurfaceSamples;
extern VkPresentModeKHR gVKPresentMode;

extern VkFormat gVKSurfaceDepthFormat;

extern mercury::u32 gAcquiredNextImageIndex;
extern VkSemaphore gSwapchainSemaphore;
extern int gSwapchainCurrentFrame;
extern int gNumberOfSwapchainFrames;
extern bool gSwapchainNeedRebuild;

struct FrameInFlight
{
	VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
	VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
	VkFence inFlightFence = VK_NULL_HANDLE;
	VkFramebuffer framebuffer = VK_NULL_HANDLE;
	VkImage image = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkImage depthImage = VK_NULL_HANDLE;
	VkImageView depthImageView = VK_NULL_HANDLE;
	VkImage msaaImage = VK_NULL_HANDLE;
	VkImageView msaaImageView = VK_NULL_HANDLE;

	VmaAllocation depthAllocation = VK_NULL_HANDLE;
	VmaAllocation msaaAllocation = VK_NULL_HANDLE;
};

extern std::vector<FrameInFlight> gFramesInFlight;

#endif