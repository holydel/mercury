#pragma once
#include "mercury_vulkan.h"

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include "ll/graphics.h"

//constants
constexpr mercury::u32 Ver11 = VK_MAKE_VERSION(1, 1, 0);
constexpr mercury::u32 Ver12 = VK_MAKE_VERSION(1, 2, 0);
constexpr mercury::u32 Ver13 = VK_MAKE_VERSION(1, 3, 0);
constexpr mercury::u32 Ver14 = VK_MAKE_VERSION(1, 4, 0);
constexpr mercury::u32 Ver15 = VK_MAKE_VERSION(1, 5, 0);

//instance
extern mercury::u32 gPhysicalDeviceAPIVersion;
extern VkInstance gVKInstance;
extern VkAllocationCallbacks* gVKGlobalAllocationsCallbacks;
extern VkDebugUtilsMessengerEXT gVKDebugMessenger;
extern std::vector<VkPhysicalDevice> gAllPhysicalDevices;

//adapter
extern VkPhysicalDevice gVKPhysicalDevice;

//device
extern VkDevice gVKDevice;
extern VkQueue gVKGraphicsQueue;
extern VkQueue gVKTransferQueue;
extern VkQueue gVKComputeQueue;

extern mercury::Config::VKConfig gVKConfig;

extern VkRenderPass gVKFinalRenderPass;

extern VmaAllocator gVMA_Allocator;
#endif
