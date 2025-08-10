#include "ll/graphics.h"

#if defined(MERCURY_LL_GRAPHICS_VULKAN)

using namespace mercury;
using namespace mercury::ll::graphics;

#include "vk_graphics.h"

VkDevice gVKDevice = VK_NULL_HANDLE;
VkQueue gVKGraphicsQueue = VK_NULL_HANDLE;
VkQueue gVKTransferQueue = VK_NULL_HANDLE;
VkQueue gVKComputeQueue = VK_NULL_HANDLE;

void Device::Initialize()
{
    MLOG_DEBUG(u8"Initialize Device (Vulkan)");

    auto &config = mercury::Application::GetCurrentApplication()->GetConfig();

    VKDeviceExtender device_extender(gVKPhysicalDevice, gPhysicalDeviceAPIVersion);

	auto queueFamilies = EnumerateVulkanObjects(gVKPhysicalDevice, vkGetPhysicalDeviceQueueFamilyProperties);


    	for (u32 i = 0; i < queueFamilies.size(); ++i)
	{
		VkQueueFamilyProperties queueFamily = queueFamilies[i];
		bool isGraphicsQueue = queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT;
		bool isTransferQueue = queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT;
		bool isComputeQueue = queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT;
		bool isSparseBindingQueue = queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;

		bool isVideoDecodeQueue = queueFamily.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR;
		bool isVideoEncodeQueue = queueFamily.queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR;

		bool isOpticalFlowNVQueue = queueFamily.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV;

		bool isProtectedQueue = queueFamily.queueFlags & VK_QUEUE_PROTECTED_BIT;

#ifdef ERMY_OS_WINDOWS
		bool supportPresent = vkGetPhysicalDeviceWin32PresentationSupportKHR != nullptr ? vkGetPhysicalDeviceWin32PresentationSupportKHR(gVKPhysicalDevice, i) : false;
#else
		bool supportPresent = false;
#endif

		MLOG_DEBUG(u8"queueID: %d (%d) image gran (%d x %d x %d) %s%s%s%s%s%s%s%s",
			i, queueFamily.queueCount,
			queueFamily.minImageTransferGranularity.width,
			queueFamily.minImageTransferGranularity.height,
			queueFamily.minImageTransferGranularity.depth,
			isGraphicsQueue ? "GRAPHICS " : "",
			isTransferQueue ? "TRANSFER " : "",
			isComputeQueue ? "COMPUTE " : "",
			isSparseBindingQueue ? "SPARSE_BINDING " : "",
			isVideoDecodeQueue ? "VIDEO_DECODE " : "",
			isVideoEncodeQueue ? "VIDEO_ENCODE " : "",
			isOpticalFlowNVQueue ? "OPTICAL_FLOW_NV " : "",
			isProtectedQueue ? "PROTECTED " : "",
			supportPresent ? "PRESENT" : "");

	}

	VkPhysicalDeviceMemoryProperties memProps = {};
	vkGetPhysicalDeviceMemoryProperties(gVKPhysicalDevice, &memProps);

	for (u32 i = 0; i < memProps.memoryHeapCount; ++i)
	{
		MLOG_DEBUG(u8"HEAP(%d) SIZE: %d MB %s", i, (memProps.memoryHeaps[i].size / (1024 * 1024)),
			memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ? "DEVICE" : "HOST");
	}

	for (u32 i = 0; i < memProps.memoryTypeCount; ++i)
	{
		std::string desc = "MEM TYPE (" + std::to_string(i) + ") HEAP: " + std::to_string(memProps.memoryTypes[i].heapIndex);

		if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			desc += "| DEVICE_LOCAL_BIT";

		if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
			desc += "| HOST_VISIBLE_BIT";

		if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			desc += "| HOST_COHERENT_BIT";

		if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
			desc += "| HOST_CACHED_BIT";

		if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
			desc += "| LAZILY_ALLOCATED_BIT";

		if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
			desc += "| PROTECTED_BIT";

		if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
			desc += "| DEVICE_COHERENT_BIT_AMD";

		if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
			desc += "| DEVICE_UNCACHED_BIT_AMD";

		if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV)
			desc += "| RDMA_CAPABLE_BIT_NV";

		MLOG_DEBUG((const char8_t*)desc.c_str());
	}
}

void Device::Shutdown()
{
    MLOG_DEBUG(u8"Shutdown Device (Vulkan)");
}

void Device::Tick()
{
    // MLOG_DEBUG(u8"Tick Device (NULL)");
}

void Device::InitializeSwapchain(void *native_window_handle)
{
    if (gSwapchain != nullptr)
    {
        MLOG_DEBUG(u8"Swapchain already initialized, skipping.");
        return;
    }

    gSwapchain = new Swapchain();
    gSwapchain->Initialize(native_window_handle);
}

void Device::ShutdownSwapchain()
{
    if (gSwapchain)
    {
        gSwapchain->Shutdown();
        delete gSwapchain;
        gSwapchain = nullptr;
    }
}

void *Device::GetNativeHandle()
{
    return nullptr; // null device has no native handle
}

void Adapter::CreateDevice()
{
    MERCURY_ASSERT(gDevice == nullptr);

    gDevice = new Device();
}

#endif