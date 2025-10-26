#include "vk_graphics.h"

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include "mercury_log.h"
#include "mercury_application.h"
#include "../../../graphics.h"
#include "vk_swapchain.h"
#include "vk_utils.h"
#include <array>
#include <algorithm>

using namespace mercury;
using namespace mercury::ll::graphics;

struct SwapchainExt
{
	// instance extensions
	bool hasSurfaceCapabilities2 : 1 = false;
	bool hasSurfaceMaintenance1 : 1 = false;
	bool hasSwapchainColorspace : 1 = false;
	bool hasDisplay : 1 = false;
	bool hasDisplay2Props : 1 = false;
	bool hasDirectModeDisplay : 1 = false;
	bool hasProtectedCapabilities : 1 = false;
	// device extensions
	bool hasMaintenance1 : 1 = false;
	bool hasPresentId : 1 = false;
	bool hasPresentWait : 1 = false;
	bool hasGoogleTiming : 1 = false;
	bool hasExclusiveFullscreen : 1 = false;
	bool hasHDRMetadata : 1 = false;
	bool hasNVAcquireDisplay : 1 = false;
} gVkSwapchainEXT;

void vkSwapchainRequestInstanceExtensions(VKInstanceExtender &instanceExtender)
{
	instanceExtender.TryAddExtension(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef MERCURY_LL_OS_WIN32
	instanceExtender.TryAddExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#ifdef MERCURY_LL_OS_ANDROID
	instanceExtender.TryAddExtension(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif
#ifdef ALLOW_XCB_SURFACE
	instanceExtender.TryAddExtension(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
#ifdef ALLOW_WAYLAND_SURFACE
	instanceExtender.TryAddExtension(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif

#ifdef MERCURY_LL_OS_LINUX
	// VK_KHR_xcb_surface
	
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

void vkSwapchainRequestDeviceExtensions(VKDeviceExtender &device_extender)
{
	device_extender.TryAddExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME); // TODO: fill device info with supported features

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

VkSwapchainKHR gVKSwapchain = VK_NULL_HANDLE;
VkSwapchainKHR gVKOldSwapchain = VK_NULL_HANDLE;
VkSurfaceKHR gVKSurface = VK_NULL_HANDLE;
VkSurfaceCapabilitiesKHR gVKSurfaceCaps;
VkFormat gVKSurfaceFormat = VK_FORMAT_UNDEFINED;
VkSampleCountFlagBits gVKSurfaceSamples = VK_SAMPLE_COUNT_1_BIT;
VkPresentModeKHR gVKPresentMode = VK_PRESENT_MODE_FIFO_KHR;

VkFormat gVKSurfaceDepthFormat = VK_FORMAT_UNDEFINED;

u32 gAcquiredNextImageIndex = 0;
VkSemaphore gSwapchainSemaphore = VK_NULL_HANDLE;
int gSwapchainCurrentFrame = 0;
int gNumberOfSwapchainFrames = 2;
bool gSwapchainNeedRebuild = false;
std::vector<FrameInFlight> gFramesInFlight;

VkSemaphore gFrameGraphSemaphore = VK_NULL_HANDLE;
u32 gFrameRingCurrent{0};

struct FrameData
{
	VkCommandPool cmdPool = VK_NULL_HANDLE;
	VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
	u64 frameIndex = 0;
};

std::vector<FrameData> gFrames;

void InitVkSwapchainResources()
{
	u32 imageCount = 0;
	vkGetSwapchainImagesKHR(gVKDevice, gVKSwapchain, &imageCount, nullptr);
	std::vector<VkImage> images(imageCount);
	vkGetSwapchainImagesKHR(gVKDevice, gVKSwapchain, &imageCount, images.data());

	gFramesInFlight.resize(imageCount);
	for (int i = 0; i < gNumberOfSwapchainFrames; ++i)
	{
		if (gVKSurfaceDepthFormat != VK_FORMAT_UNDEFINED)
		{
			VkImageCreateInfo depthImageInfo{};
			depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
			depthImageInfo.format = gVKSurfaceDepthFormat; // e.g., VK_FORMAT_D32_SFLOAT
			depthImageInfo.extent = {gVKSurfaceCaps.currentExtent.width, gVKSurfaceCaps.currentExtent.height, 1};
			depthImageInfo.mipLevels = 1;
			depthImageInfo.arrayLayers = 1;
			depthImageInfo.samples = gVKSurfaceSamples; // Match MSAA sample count
			depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			VmaAllocationCreateInfo depthAllocInfo{};
			depthAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

			vmaCreateImage(gVMA_Allocator, &depthImageInfo, &depthAllocInfo,
						   &gFramesInFlight[i].depthImage, &gFramesInFlight[i].depthAllocation, nullptr);
			vk_utils::debug::SetName(gFramesInFlight[i].depthImage, "Depth Image (%d)", i);

			VkImageViewCreateInfo depthViewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
			depthViewInfo.image = gFramesInFlight[i].depthImage;
			depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			depthViewInfo.format = gVKSurfaceDepthFormat;
			depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			depthViewInfo.subresourceRange.baseMipLevel = 0;
			depthViewInfo.subresourceRange.levelCount = 1;
			depthViewInfo.subresourceRange.baseArrayLayer = 0;
			depthViewInfo.subresourceRange.layerCount = 1;

			vkCreateImageView(gVKDevice, &depthViewInfo, nullptr, &gFramesInFlight[i].depthImageView);
			vk_utils::debug::SetName(gFramesInFlight[i].depthImageView, "Depth Image View (%d)", i);
		}

		// MSAA Image Creation with VMA
		if (gVKSurfaceSamples != VK_SAMPLE_COUNT_1_BIT)
		{
			VkImageCreateInfo msaaImageInfo{};
			msaaImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			msaaImageInfo.imageType = VK_IMAGE_TYPE_2D;
			msaaImageInfo.format = gVKSurfaceFormat;
			msaaImageInfo.extent = {gVKSurfaceCaps.currentExtent.width, gVKSurfaceCaps.currentExtent.height, 1};
			msaaImageInfo.mipLevels = 1;
			msaaImageInfo.arrayLayers = 1;
			msaaImageInfo.samples = gVKSurfaceSamples; // e.g., VK_SAMPLE_COUNT_4_BIT
			msaaImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			msaaImageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			msaaImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			VmaAllocationCreateInfo msaaAllocInfo{};
			msaaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

			vmaCreateImage(gVMA_Allocator, &msaaImageInfo, &msaaAllocInfo,
						   &gFramesInFlight[i].msaaImage, &gFramesInFlight[i].msaaAllocation, nullptr);
			vk_utils::debug::SetName(gFramesInFlight[i].msaaImage, "MSAA Image (%d)", i);

			VkImageViewCreateInfo msaaViewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
			msaaViewInfo.image = gFramesInFlight[i].msaaImage;
			msaaViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			msaaViewInfo.format = gVKSurfaceFormat;
			msaaViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			msaaViewInfo.subresourceRange.baseMipLevel = 0;
			msaaViewInfo.subresourceRange.levelCount = 1;
			msaaViewInfo.subresourceRange.baseArrayLayer = 0;
			msaaViewInfo.subresourceRange.layerCount = 1;

			vkCreateImageView(gVKDevice, &msaaViewInfo, nullptr, &gFramesInFlight[i].msaaImageView);
			vk_utils::debug::SetName(gFramesInFlight[i].msaaImageView, "MSAA Image View (%d)", i);
		}

		gFramesInFlight[i].image = images[i];
		gFramesInFlight[i].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED; // track initial layout
		vk_utils::debug::SetName(gFramesInFlight[i].image, "Swapchain Target (%d)", i);

		VkImageViewCreateInfo imageViewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
		imageViewInfo.image = images[i];
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = gVKSurfaceFormat;
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;

		vkCreateImageView(gVKDevice, &imageViewInfo, nullptr, &gFramesInFlight[i].imageView);
		vk_utils::debug::SetName(gFramesInFlight[i].imageView, "Swapchain Target View (%d)", i);

		VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		vkCreateSemaphore(gVKDevice, &semaphoreInfo, nullptr, &gFramesInFlight[i].imageAvailableSemaphore);
		vkCreateSemaphore(gVKDevice, &semaphoreInfo, nullptr, &gFramesInFlight[i].renderFinishedSemaphore);
		if (!gVKConfig.useDynamicRendering)
		{
			VkImageView allViews[3] = {VK_NULL_HANDLE};
			int currentViewIndex = 0;

			if (gVKSurfaceDepthFormat != VK_FORMAT_UNDEFINED)
			{
				allViews[currentViewIndex] = gFramesInFlight[i].depthImageView;
				currentViewIndex++;
			}

			if (gVKSurfaceSamples != VK_SAMPLE_COUNT_1_BIT)
			{
				allViews[currentViewIndex] = gFramesInFlight[i].msaaImageView;
				currentViewIndex++;
			}

			allViews[currentViewIndex] = gFramesInFlight[i].imageView;
			currentViewIndex++;

			VkFramebufferCreateInfo framebufferInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
			framebufferInfo.renderPass = gVKFinalRenderPass;
			framebufferInfo.attachmentCount = currentViewIndex;
			framebufferInfo.pAttachments = allViews;
			framebufferInfo.width = gVKSurfaceCaps.currentExtent.width;
			framebufferInfo.height = gVKSurfaceCaps.currentExtent.height;
			framebufferInfo.layers = 1;

			vkCreateFramebuffer(gVKDevice, &framebufferInfo, nullptr, &gFramesInFlight[i].framebuffer);

			vk_utils::debug::SetName(gFramesInFlight[i].framebuffer, "Swapchain Target Framebuffer (%d)", i);
		}
	}

	gSwapchainCurrentFrame = 0;
}

void ShutdownVkSwapchainResources()
{
	for (auto &frame : gFramesInFlight)
	{
		vkDestroySemaphore(gVKDevice, frame.imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(gVKDevice, frame.renderFinishedSemaphore, nullptr);
		vkDestroyFramebuffer(gVKDevice, frame.framebuffer, nullptr);
		vkDestroyImageView(gVKDevice, frame.imageView, nullptr);

		if (frame.depthImage != VK_NULL_HANDLE)
		{
			vkDestroyImageView(gVKDevice, frame.depthImageView, nullptr);
			vmaDestroyImage(gVMA_Allocator, frame.depthImage, frame.depthAllocation);
		}
		if (frame.msaaImage != VK_NULL_HANDLE)
		{
			vkDestroyImageView(gVKDevice, frame.msaaImageView, nullptr);
			vmaDestroyImage(gVMA_Allocator, frame.msaaImage, frame.msaaAllocation);
		}
	}
}

VkCompositeAlphaFlagBitsKHR CalculateCompositeAlphaFlag()
{
	if (gVKSurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
		return VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;

	if (gVKSurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
		return VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;

	if (gVKSurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
		return VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

	return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

void InitVkSwapchain()
{
	gVKOldSwapchain = gVKSwapchain;
	VkSwapchainCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
	createInfo.clipped = VK_TRUE;
	createInfo.imageArrayLayers = 1;
	createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	createInfo.imageExtent = gVKSurfaceCaps.currentExtent;
	createInfo.imageFormat = gVKSurfaceFormat;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.minImageCount = gNumberOfSwapchainFrames;
	createInfo.presentMode = gVKPresentMode;
	createInfo.surface = gVKSurface;
	createInfo.preTransform = gVKSurfaceCaps.currentTransform;
	createInfo.oldSwapchain = gVKOldSwapchain;
	createInfo.clipped = VK_TRUE;
	createInfo.compositeAlpha = CalculateCompositeAlphaFlag();

	VkSwapchainPresentModesCreateInfoEXT presentationModesEnabled = {VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODES_CREATE_INFO_EXT};
	presentationModesEnabled.presentModeCount = 1;
	presentationModesEnabled.pPresentModes = &gVKPresentMode;

	if (gVkSwapchainEXT.hasMaintenance1)
	{
		createInfo.pNext = &presentationModesEnabled;
	}

	VK_CALL(vkCreateSwapchainKHR(gVKDevice, &createInfo, nullptr, &gVKSwapchain));
	vk_utils::debug::SetName(gVKSwapchain, "Main Window Swapchain");
}

void ShutdownVkSwapchain()
{
	vkDestroySwapchainKHR(gVKDevice, gVKSwapchain, nullptr);
}
void ShutdownOldVkSwapchain()
{
	vkDestroySwapchainKHR(gVKDevice, gVKOldSwapchain, nullptr);
}

void *Swapchain::GetNativeHandle()
{
	return gVKSwapchain;
}

void CalculateSwapChainHeuristics(const std::vector<VkSurfaceFormatKHR> &supportFormats, const std::vector<VkPresentModeKHR> &supportedPresentModes)
{
	const auto &swapChainSettings = mercury::Application::GetCurrentApplication()->GetConfig().swapchain;

	const VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo2{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
													   .surface = gVKSurface};
	VkSurfaceCapabilities2KHR capabilities2{.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};
	vkGetPhysicalDeviceSurfaceCapabilities2KHR(gVKPhysicalDevice, &surfaceInfo2, &capabilities2);

	gNumberOfSwapchainFrames = std::max(swapChainSettings.tripleBuffering ? 3u : 2u, capabilities2.surfaceCapabilities.minImageCount);

	VkPresentModeKHR preferredModes[3][7] = {
		{// NoVSync
		 VK_PRESENT_MODE_MAILBOX_KHR,
		 VK_PRESENT_MODE_FIFO_LATEST_READY_EXT,
		 VK_PRESENT_MODE_IMMEDIATE_KHR,
		 VK_PRESENT_MODE_FIFO_RELAXED_KHR,
		 VK_PRESENT_MODE_FIFO_KHR,
		 VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR,
		 VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR},
		{// AdaptiveVSync
		 VK_PRESENT_MODE_FIFO_LATEST_READY_EXT,
		 VK_PRESENT_MODE_FIFO_RELAXED_KHR,
		 VK_PRESENT_MODE_MAILBOX_KHR,
		 VK_PRESENT_MODE_FIFO_KHR,
		 VK_PRESENT_MODE_IMMEDIATE_KHR,
		 VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR,
		 VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR},
		{// AlwaysVSync
		 VK_PRESENT_MODE_FIFO_KHR,
		 VK_PRESENT_MODE_FIFO_LATEST_READY_EXT,
		 VK_PRESENT_MODE_FIFO_RELAXED_KHR,
		 VK_PRESENT_MODE_MAILBOX_KHR,
		 VK_PRESENT_MODE_IMMEDIATE_KHR,
		 VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR,
		 VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR},
	};

	std::vector<VkFormat> formatsFor16Bit = {
		VK_FORMAT_R5G6B5_UNORM_PACK16,
		VK_FORMAT_B5G6R5_UNORM_PACK16,
		VK_FORMAT_R5G5B5A1_UNORM_PACK16,
		VK_FORMAT_B5G5R5A1_UNORM_PACK16,
		VK_FORMAT_A1R5G5B5_UNORM_PACK16,
		VK_FORMAT_R4G4B4A4_UNORM_PACK16,
		VK_FORMAT_B4G4R4A4_UNORM_PACK16,
	};

	std::vector<VkFormat> formatsFor32Bit = {
		VK_FORMAT_A2R10G10B10_UNORM_PACK32,
		VK_FORMAT_A2B10G10R10_UNORM_PACK32,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_B8G8R8A8_SRGB,
	};

	std::vector<VkFormat> formatsForHDR = {
		VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
		VK_FORMAT_B10G11R11_UFLOAT_PACK32,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R16G16B16A16_UNORM,
		VK_FORMAT_R32G32B32A32_SFLOAT};

	for (int i = 0; i < 7; ++i)
	{
		auto desiredPresentMode = preferredModes[(int)swapChainSettings.vsync][i];
		if (std::find(supportedPresentModes.begin(), supportedPresentModes.end(), desiredPresentMode) != supportedPresentModes.end())
		{
			gVKPresentMode = desiredPresentMode;
			break;
		}
	}

	std::vector<std::vector<VkFormat>> formatsForBitDepths;
	formatsForBitDepths.reserve(3);

	if (swapChainSettings.colorWidth == mercury::Config::SwapchainConfig::ColorWidth::Prefer16bit)
	{
		formatsForBitDepths.push_back(formatsFor16Bit);
		formatsForBitDepths.push_back(formatsFor32Bit);
		formatsForBitDepths.push_back(formatsForHDR);
	}

	if (swapChainSettings.colorWidth == mercury::Config::SwapchainConfig::ColorWidth::Prefer32bit)
	{
		formatsForBitDepths.push_back(formatsFor32Bit);
		formatsForBitDepths.push_back(formatsForHDR);
		formatsForBitDepths.push_back(formatsFor16Bit);
	}

	if (swapChainSettings.colorWidth == mercury::Config::SwapchainConfig::ColorWidth::PreferHDR)
	{
		formatsForBitDepths.push_back(formatsForHDR);
		formatsForBitDepths.push_back(formatsFor32Bit);
		formatsForBitDepths.push_back(formatsFor16Bit);
	}

	gVKSurfaceFormat = VK_FORMAT_UNDEFINED;

	for (auto &formatList : formatsForBitDepths)
	{
		for (auto &format : formatList)
		{
			auto it = std::find_if(supportFormats.begin(), supportFormats.end(), [format](const VkSurfaceFormatKHR &supportedFormat)
								   { return supportedFormat.format == format; });

			if (it != supportFormats.end())
			{
				gVKSurfaceFormat = it->format;
				break;
			}
		}
		if (gVKSurfaceFormat != VK_FORMAT_UNDEFINED)
			break;
	}

	auto preferredMSAAMode = swapChainSettings.msaaMode;
	auto preferredDepthMode = swapChainSettings.depthMode;
	gVKSurfaceSamples = (VkSampleCountFlagBits)preferredMSAAMode;

	{
		using mode = mercury::Config::SwapchainConfig::DepthMode;
		if (preferredDepthMode == mode::Depth16)
			gVKSurfaceDepthFormat = VK_FORMAT_D16_UNORM;
		if (preferredDepthMode == mode::Depth24_Stencil8)
			gVKSurfaceDepthFormat = VK_FORMAT_D24_UNORM_S8_UINT;
		if (preferredDepthMode == mode::Depth32F)
			gVKSurfaceDepthFormat = VK_FORMAT_D32_SFLOAT;
		if (preferredDepthMode == mode::Depth32F_Stencil8)
			gVKSurfaceDepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
	}
}

void Swapchain::Initialize()
{
	auto os = mercury::ll::os::gOS;

	gVKSurface = (VkSurfaceKHR)os->CreateVkSurface(gVKInstance,gVKGlobalAllocationsCallbacks);
/*
#ifdef MERCURY_LL_OS_WIN32
	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = static_cast<HWND>(native_window_handle);
	createInfo.hinstance = static_cast<HINSTANCE>(GetModuleHandleA(nullptr));

	VK_CALL(vkCreateWin32SurfaceKHR(gVKInstance, &createInfo, nullptr, &gVKSurface));
#endif

#ifdef MERCURY_LL_OS_ANDROID
	VkAndroidSurfaceCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, nullptr};

	createInfo.flags = 0;
	createInfo.window = static_cast<ANativeWindow *>(os::GetNativeWindowHandle());

	vkCreateAndroidSurfaceKHR(gVKInstance, &createInfo, nullptr, &gVKSurface);
#endif

#ifdef MERCURY_LL_OS_LINUX
	
	#ifdef ALLOW_WAYLAND_SURFACE
	VkWaylandSurfaceCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
	createInfo.surface = (wl_surface*)native_window_handle;
	createInfo.display 
	#endif
	//VkXcbSurfaceCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
	//createInfo.window = *((xcb_window_t *)native_window_handle);
	//createInfo.connection = (xcb_connection_t *)mercury::platform::getAppInstanceHandle();

	//vkCreateXcbSurfaceKHR(gInstance, &createInfo, gGlobalAllocationsCallbacks, &gSurface);
#endif

#ifdef MERCURY_LL_OS_MACOS
	VkMetalSurfaceCreateInfoEXT createInfo{VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT};
	createInfo.pLayer = static_cast<CAMetalLayer *>(nativeWindowHandle);

	vkCreateMetalSurfaceEXT(gInstance, &createInfo, gGlobalAllocationsCallbacks, &gSurface);
#endif
*/
	VkBool32 is_supported = false;

	vkGetPhysicalDeviceSurfaceSupportKHR(gVKPhysicalDevice, 0, gVKSurface, &is_supported);

	if (!is_supported)
	{
		MLOG_ERROR(u8"Swapchain surface doesn't support!");
		return;
	}

	auto support_formats = EnumerateVulkanObjects(gVKPhysicalDevice, gVKSurface, vkGetPhysicalDeviceSurfaceFormatsKHR);
	auto support_present_modes = EnumerateVulkanObjects(gVKPhysicalDevice, gVKSurface, vkGetPhysicalDeviceSurfacePresentModesKHR);

	VK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gVKPhysicalDevice, gVKSurface, &gVKSurfaceCaps));

	if(gVKSurfaceCaps.currentExtent.width == 0xFFFFFFFFu)
	{
		os->GetActualWindowSize(gVKSurfaceCaps.currentExtent.width, gVKSurfaceCaps.currentExtent.height);
	}
	
	MLOG_INFO(u8"Supported surface alpha modes: %s %s %s %s", (gVKSurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) ? " Opaque |" : "", (gVKSurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) ? " Inherit |" : "", (gVKSurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) ? " Post Multiplied |" : "", (gVKSurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) ? " Pre Multiplied |" : "");

	CalculateSwapChainHeuristics(support_formats, support_present_modes);

	if (!gVKConfig.useDynamicRendering)
	{
		VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};

		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference> attachmentRefs;
		int depthAttachmentIndex = -1;
		int msaaAttachmentIndex = -1;
		int finalAttachmentIndex = 0;

		if (gVKSurfaceDepthFormat != VK_FORMAT_UNDEFINED)
		{
			VkAttachmentDescription depthAttachment{};
			depthAttachment.format = gVKSurfaceDepthFormat;
			depthAttachment.samples = gVKSurfaceSamples;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			depthAttachmentIndex = (i32)attachments.size();
			attachments.push_back(depthAttachment);
		}

		if (gVKSurfaceSamples != VK_SAMPLE_COUNT_1_BIT)
		{
			VkAttachmentDescription msaaColorAttachment{};
			msaaColorAttachment.format = gVKSurfaceFormat;
			msaaColorAttachment.samples = gVKSurfaceSamples;
			msaaColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			msaaColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			msaaColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			msaaColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			msaaColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			msaaColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			msaaAttachmentIndex = (i32)attachments.size();
			attachments.push_back(msaaColorAttachment);
		}

		{
			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = gVKSurfaceFormat;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = gVKSurfaceSamples != VK_SAMPLE_COUNT_1_BIT ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			finalAttachmentIndex = (i32)attachments.size();
			attachments.push_back(colorAttachment);
		}

		VkAttachmentReference finalAttachmentRef{};
		finalAttachmentRef.attachment = finalAttachmentIndex;
		finalAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference msaaColorAttachmentRef{};
		msaaColorAttachmentRef.attachment = msaaAttachmentIndex;
		msaaColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = depthAttachmentIndex;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;

		if (depthAttachmentIndex >= 0)
		{
			subpass.pDepthStencilAttachment = &depthAttachmentRef;
		}

		if (msaaAttachmentIndex >= 0)
		{
			subpass.pColorAttachments = &msaaColorAttachmentRef;
			subpass.pResolveAttachments = &finalAttachmentRef;
		}
		else
		{
			subpass.pColorAttachments = &finalAttachmentRef;
		}

		// These dependencies ensure proper synchronization
		std::array<VkSubpassDependency, 2> dependencies;
		//

		dependencies[0] = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0};
		dependencies[1] = {
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
			.dependencyFlags = 0};

		renderPassInfo.attachmentCount = (u32)attachments.size();
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = static_cast<u32>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		vkCreateRenderPass(gVKDevice, &renderPassInfo, nullptr, &gVKFinalRenderPass);

		vk_utils::debug::SetName(gVKFinalRenderPass, "Final RenderPass");

		InitVkSwapchain();
		InitVkSwapchainResources();

		const uint64_t initialValue = (gNumberOfSwapchainFrames - 1);

		VkSemaphoreTypeCreateInfo timelineCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
			.pNext = nullptr,
			.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
			.initialValue = initialValue,
		};

		const VkSemaphoreCreateInfo semaphoreCreateInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = &timelineCreateInfo};
		VK_CALL(vkCreateSemaphore(gVKDevice, &semaphoreCreateInfo, nullptr, &gFrameGraphSemaphore));

		vk_utils::debug::SetName(gFrameGraphSemaphore, "FrameGraph semaphore");

		gFrames.resize(gNumberOfSwapchainFrames);

		const VkCommandPoolCreateInfo cmdPoolCreateInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.queueFamilyIndex = 0, // TODO: get from device
		};

		for (u8 i = 0; i < gNumberOfSwapchainFrames; ++i)
		{
			auto &f = gFrames[i];
			f.frameIndex = i; // Track frame index for synchronization
			VK_CALL(vkCreateCommandPool(gVKDevice, &cmdPoolCreateInfo, nullptr, &f.cmdPool));

			vk_utils::debug::SetName(f.cmdPool, "Frame Command Pool (%d)", i);

			const VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = f.cmdPool,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = 1,
			};
			VK_CALL(vkAllocateCommandBuffers(gVKDevice, &commandBufferAllocateInfo, &f.cmdBuffer));

			vk_utils::debug::SetName(f.cmdBuffer, "Frame Command Buffer (%d)", i);
		}
	}
}

void Swapchain::Resize(u16 width, u16 height)
{
	gSwapchainNeedRebuild = true;
}

void Swapchain::ReInitIfNeeded()
{
	if (gFramesInFlight.empty())
		return;

	if (gSwapchainNeedRebuild)
	{
		vkDeviceWaitIdle(gVKDevice);
		VK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gVKPhysicalDevice, gVKSurface, &gVKSurfaceCaps));

		ShutdownVkSwapchainResources();
		InitVkSwapchain();
		ShutdownOldVkSwapchain();
		InitVkSwapchainResources();

		gSwapchainNeedRebuild = false;
	}
}

void Swapchain::Shutdown()
{
	MLOG_DEBUG(u8"Shutdown Swapchain (Vulkan)");

	ShutdownVkSwapchainResources();
	ShutdownVkSwapchain();
	ShutdownOldVkSwapchain();

	vkDestroySemaphore(gVKDevice, gFrameGraphSemaphore, nullptr);
	gFrameGraphSemaphore = VK_NULL_HANDLE;
	for (auto &frame : gFrames)
	{
		vkDestroyCommandPool(gVKDevice, frame.cmdPool, nullptr);
		frame.cmdPool = VK_NULL_HANDLE;
		frame.cmdBuffer = VK_NULL_HANDLE;
	}
	gFrames.clear();
}

CommandList Swapchain::AcquireNextImage()
{
	ReInitIfNeeded();

	auto &frame = gFramesInFlight[gSwapchainCurrentFrame];
	auto &frameCPU = gFrames[gFrameRingCurrent];
	CommandList outCbuff = CommandList(frameCPU.cmdBuffer);
	if (gFramesInFlight.empty())
		return outCbuff;

	const uint64_t waitValue = frameCPU.frameIndex;
	const VkSemaphoreWaitInfo waitInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
		.semaphoreCount = 1,
		.pSemaphores = &gFrameGraphSemaphore,
		.pValues = &waitValue,
	};

	vkWaitSemaphores(gVKDevice, &waitInfo, std::numeric_limits<uint64_t>::max());

	auto result = vkAcquireNextImageKHR(gVKDevice, gVKSwapchain, UINT64_MAX, frame.imageAvailableSemaphore, VK_NULL_HANDLE, &gAcquiredNextImageIndex);

	VK_CALL(vkResetCommandPool(gVKDevice, frameCPU.cmdPool, 0));

	VkCommandBuffer cmd = frameCPU.cmdBuffer;
	// Begin the command buffer recording for the frame
	const VkCommandBufferBeginInfo beginInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
											 .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
	VK_CALL(vkBeginCommandBuffer(cmd, &beginInfo));

	// IMPORTANT: operate on the acquired image/resources
	auto &imageFrame = gFramesInFlight[gAcquiredNextImageIndex];

	if (gVKConfig.useDynamicRendering)
	{
		VkClearValue clearValue;
		clearValue.color = {clearColor.x, clearColor.y, clearColor.z, clearColor.w};

		vk_utils::ImageTransition(frameCPU.cmdBuffer, imageFrame.image, imageFrame.imageLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

		VkRenderingAttachmentInfo colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
		colorAttachment.imageView = imageFrame.imageView;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.clearValue = clearValue;

		VkRenderingInfo renderInfo{VK_STRUCTURE_TYPE_RENDERING_INFO};
		renderInfo.renderArea = {{0, 0}, {gVKSurfaceCaps.currentExtent.width, gVKSurfaceCaps.currentExtent.height}};
		renderInfo.layerCount = 1;
		renderInfo.colorAttachmentCount = 1;
		renderInfo.pColorAttachments = &colorAttachment;

		vkCmdBeginRendering(frameCPU.cmdBuffer, &renderInfo);
	}
	else
	{
		if (imageFrame.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED)
		{
			vk_utils::ImageTransition(cmd, imageFrame.image, imageFrame.imageLayout, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT);
		}

		VkClearValue clearValues[3] = {};
		int colorAttachmentIndex = 0;

		if (gVKSurfaceDepthFormat != VK_FORMAT_UNDEFINED)
		{
			clearValues[0].depthStencil.depth = clearDepth;
			clearValues[0].depthStencil.stencil = clearStencil;
			colorAttachmentIndex++;
		}

		for (int i = 0; i < 4; ++i)
			clearValues[colorAttachmentIndex].color.float32[i] = clearColor[i];

		colorAttachmentIndex++;

		VkRenderPassBeginInfo rpass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
		rpass.clearValueCount = colorAttachmentIndex;
		rpass.pClearValues = clearValues;
		rpass.framebuffer = imageFrame.framebuffer;
		rpass.renderArea = {0, 0, gVKSurfaceCaps.currentExtent.width, gVKSurfaceCaps.currentExtent.height};
		rpass.renderPass = gVKFinalRenderPass;

		vkCmdBeginRenderPass(frameCPU.cmdBuffer, &rpass, VK_SUBPASS_CONTENTS_INLINE);
	}

	return outCbuff;
}

void Swapchain::Present()
{
	if (gFramesInFlight.empty())
		return;

	auto &frame = gFramesInFlight[gSwapchainCurrentFrame];
	auto &frameCPU = gFrames[gFrameRingCurrent];

	VkCommandBuffer cmd = frameCPU.cmdBuffer;

	// Use acquired image for image ops
    auto &imageFrame = gFramesInFlight[gAcquiredNextImageIndex];

	if (gVKConfig.useDynamicRendering)
	{
		vkCmdEndRendering(cmd);
		vk_utils::ImageTransition(cmd, imageFrame.image, imageFrame.imageLayout, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	else
	{
		vkCmdEndRenderPass(cmd);
	}

	VK_CALL(vkEndCommandBuffer(cmd));

	std::vector<VkSemaphoreSubmitInfo> waitSemaphores;
	std::vector<VkSemaphoreSubmitInfo> signalSemaphores;

	waitSemaphores.push_back({
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = frame.imageAvailableSemaphore,
		.stageMask = imageFrame.imageLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ? VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
	});
	signalSemaphores.push_back({
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = frame.renderFinishedSemaphore,
		.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
	});

	/*--
	* Calculate the signal value for when this frame completes
	* Signal value = current frame number + numFramesInFlight
	* Example with 3 frames in flight:
	*   Frame 0 signals value 3 (allowing Frame 3 to start when complete)
	*   Frame 1 signals value 4 (allowing Frame 4 to start when complete)
	-*/
	const uint64_t signalFrameValue = frameCPU.frameIndex + gNumberOfSwapchainFrames;
	frameCPU.frameIndex = signalFrameValue; // Store for next time this frame buffer is used

	/*--
	* Add timeline semaphore to signal when GPU completes this frame
	* The color attachment output stage is used since that's when the frame is fully rendered
	-*/
	signalSemaphores.push_back({
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = gFrameGraphSemaphore,
		.value = signalFrameValue,
		.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
	});

	// Note : in this sample, we only have one command buffer per frame.
	const std::array<VkCommandBufferSubmitInfo, 1> cmdBufferInfo{{{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.commandBuffer = cmd,
	}}};

	// Populate the submit info to synchronize rendering and send the command buffer
	const std::array<VkSubmitInfo2, 1> submitInfo{{{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.waitSemaphoreInfoCount = uint32_t(waitSemaphores.size()),	   //
		.pWaitSemaphoreInfos = waitSemaphores.data(),				   // Wait for the image to be available
		.commandBufferInfoCount = uint32_t(cmdBufferInfo.size()),	   //
		.pCommandBufferInfos = cmdBufferInfo.data(),				   // Command buffer to submit
		.signalSemaphoreInfoCount = uint32_t(signalSemaphores.size()), //
		.pSignalSemaphoreInfos = signalSemaphores.data(),			   // Signal when rendering is finished
	}}};

	// Submit the command buffer to the GPU and signal when it's done
	VK_CALL(vkQueueSubmit2(gVKGraphicsQueue, uint32_t(submitInfo.size()), submitInfo.data(), nullptr));

	// Setup the presentation info, linking the swapchain and the image index
	const VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,						   // Wait for rendering to finish
		.pWaitSemaphores = &frame.renderFinishedSemaphore, // Synchronize presentation
		.swapchainCount = 1,							   // Swapchain to present the image
		.pSwapchains = &gVKSwapchain,					   // Pointer to the swapchain
		.pImageIndices = &gAcquiredNextImageIndex,		   // Index of the image to present
	};

	// vkQueueWaitIdle(gVKMainQueue);
	const VkResult result = vkQueuePresentKHR(gVKGraphicsQueue, &presentInfo);
	// If the swapchain is out of date (e.g., window resized), it needs to be rebuilt
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		gSwapchainNeedRebuild = true;
	}

	imageFrame.imageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	gSwapchainCurrentFrame = (gSwapchainCurrentFrame + 1) % GetNumberOfFrames();
	gFrameRingCurrent = (gFrameRingCurrent + 1) % gNumberOfSwapchainFrames;
}

u8 Swapchain::GetNumberOfFrames()
{
	return static_cast<u8>(gNumberOfSwapchainFrames);
}

void Swapchain::SetFullscreen(bool fullscreen)
{
}

int Swapchain::GetWidth() const
{
	return gVKSurfaceCaps.currentExtent.width;
}

int Swapchain::GetHeight() const
{
	return gVKSurfaceCaps.currentExtent.height;
}
#endif