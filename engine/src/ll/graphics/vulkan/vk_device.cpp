#include "ll/graphics.h"

#if defined(MERCURY_LL_GRAPHICS_VULKAN)

using namespace mercury;
using namespace mercury::ll::graphics;

#include "vk_graphics.h"
#include "vk_swapchain.h"
#include "vk_device.h"
#include "vk_utils.h"

VkDevice gVKDevice = VK_NULL_HANDLE;
VkQueue gVKGraphicsQueue = VK_NULL_HANDLE;
VkQueue gVKTransferQueue = VK_NULL_HANDLE;
VkQueue gVKComputeQueue = VK_NULL_HANDLE;
VmaAllocator gVMA_Allocator = nullptr;

DeviceEnabledExtensions gVKDeviceEnabledExtensions;

struct EnabledVKFeatures
{
	VkPhysicalDeviceVulkan11Features features11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, nullptr};
	VkPhysicalDeviceVulkan12Features features12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, nullptr};
	VkPhysicalDeviceVulkan13Features features13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, nullptr};
	VkPhysicalDeviceVulkan14Features features14 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES, nullptr};

	VkPhysicalDeviceSynchronization2Features sync2Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES, nullptr};
	VkPhysicalDeviceTimelineSemaphoreFeaturesKHR timelineSemaphoreFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR, nullptr};
	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR, nullptr};
	VkPhysicalDeviceDynamicRenderingLocalReadFeaturesKHR dynamicRenderingFeaturesLocalRead = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_LOCAL_READ_FEATURES_KHR, nullptr};

	VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR fragmentShaderBarycentricFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR};
	VkPhysicalDeviceFragmentShaderBarycentricFeaturesNV fragmentShaderBarycentricFeaturesNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_NV};

	void *BuildPChains();
	void *pchain = nullptr;

} gEnabledFeatures;

void *EnabledVKFeatures::BuildPChains()
{
	features11.multiview = true;

	features12.timelineSemaphore = true;
	timelineSemaphoreFeatures.timelineSemaphore = true;
	sync2Features.synchronization2 = true;
	features13.synchronization2 = true;

	features12.vulkanMemoryModel = true;
	features12.vulkanMemoryModelDeviceScope = true;
	features12.bufferDeviceAddress = true;
	features12.uniformAndStorageBuffer8BitAccess = true;

	if (gVKConfig.useDynamicRendering)
	{
		features13.dynamicRendering = true;
		features14.dynamicRenderingLocalRead = true;

		dynamicRenderingFeatures.dynamicRendering = true;
		dynamicRenderingFeaturesLocalRead.dynamicRenderingLocalRead = true;
	}

	if (gPhysicalDeviceAPIVersion >= Ver11)
	{
		NextPChain(pchain, &features11);
	}
	if (gPhysicalDeviceAPIVersion >= Ver12)
	{
		NextPChain(pchain, &features12);
	}
	if (gPhysicalDeviceAPIVersion >= Ver13)
	{
		NextPChain(pchain, &features13);
	}
	else
	{
		if (gVKDeviceEnabledExtensions.KhrSynchronization2)
			NextPChain(pchain, &dynamicRenderingFeaturesLocalRead);

		if (gVKDeviceEnabledExtensions.KhrSynchronization2)
			NextPChain(pchain, &sync2Features);
	}

	if (gPhysicalDeviceAPIVersion >= Ver14)
	{
		NextPChain(pchain, &features14);
	}
	else
	{
		if (gVKDeviceEnabledExtensions.KhrDynamicRenderingLocalRead)
			NextPChain(pchain, &dynamicRenderingFeaturesLocalRead);
	}

	if (gVKDeviceEnabledExtensions.KhrFragmentShaderBarycentric)
	{
		fragmentShaderBarycentricFeatures.fragmentShaderBarycentric = true;
		NextPChain(pchain, &fragmentShaderBarycentricFeatures);
	}

	if (gVKDeviceEnabledExtensions.NvFragmentShaderBarycentric)
	{
		fragmentShaderBarycentricFeaturesNV.fragmentShaderBarycentric = true;
		NextPChain(pchain, &fragmentShaderBarycentricFeaturesNV);
	}
	return pchain;
}

void Device::Initialize()
{
	MLOG_DEBUG(u8"Initialize Device (Vulkan)");

	auto &config = mercury::Application::GetCurrentApplication()->GetConfig();
	auto &graphicsCfg = config.graphics;

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

		bool supportPresent = ll::os::gOS->IsQueueSupportPresent(gVKPhysicalDevice,i);

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

		MLOG_DEBUG((const char8_t *)desc.c_str());
	}

	float highPriors[8];
	float midPriors[8];
	float lowPriors[8];

	for (int i = 0; i < 8; ++i)
	{
		highPriors[i] = 1.0f;
		midPriors[i] = 0.5f;
		lowPriors[i] = 0.0f;
	}

	VkDeviceQueueCreateInfo queueCreateInfoGraphics;
	queueCreateInfoGraphics.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfoGraphics.pNext = 0;
	queueCreateInfoGraphics.pQueuePriorities = highPriors;
	queueCreateInfoGraphics.queueCount = 1;
	queueCreateInfoGraphics.queueFamilyIndex = 0;
	queueCreateInfoGraphics.flags = 0;

	std::vector<VkDeviceQueueCreateInfo> requestedQueues;
	requestedQueues.push_back(queueCreateInfoGraphics);

	vkSwapchainRequestDeviceExtensions(device_extender);

	bool hasRenderPass2 = device_extender.TryAddExtension(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, Ver12);

	gVKDeviceEnabledExtensions.NvDecompressMemory = device_extender.TryAddExtension(VK_NV_MEMORY_DECOMPRESSION_EXTENSION_NAME);
	gVKDeviceEnabledExtensions.KhrBufferDeviceAddress = device_extender.TryAddExtension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, Ver12);
	gVKDeviceEnabledExtensions.KhrDedicatedAllocation = device_extender.TryAddExtension(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME, Ver11);
	gVKDeviceEnabledExtensions.KhrSynchronization2 = device_extender.TryAddExtension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, Ver13);
	gVKDeviceEnabledExtensions.KhrTimelineSemaphore = device_extender.TryAddExtension(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME, Ver12);
	gVKDeviceEnabledExtensions.ExtMemoryPriority = device_extender.TryAddExtension(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME);
	gVKDeviceEnabledExtensions.ExtPageableDeviceLocalMemory = device_extender.TryAddExtension(VK_EXT_PAGEABLE_DEVICE_LOCAL_MEMORY_EXTENSION_NAME);

	VkPhysicalDeviceFeatures enabledFeatures10 = {};
	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(gVKPhysicalDevice, &supportedFeatures);

	if (gVKConfig.useDynamicRendering)
	{
		gVKDeviceEnabledExtensions.KhrDynamicRendering = device_extender.TryAddExtension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, Ver13);
		gVKDeviceEnabledExtensions.KhrDynamicRenderingLocalRead = device_extender.TryAddExtension(VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME, Ver14);
	}

	if (graphicsCfg.enableBarycentricFS)
	{
		gVKDeviceEnabledExtensions.KhrFragmentShaderBarycentric = device_extender.TryAddExtension(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME);
		if (!gVKDeviceEnabledExtensions.KhrFragmentShaderBarycentric)
		{
			gVKDeviceEnabledExtensions.NvFragmentShaderBarycentric = device_extender.TryAddExtension(VK_NV_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME);
		}

		graphicsCfg.enableBarycentricFS = gVKDeviceEnabledExtensions.KhrFragmentShaderBarycentric || gVKDeviceEnabledExtensions.NvFragmentShaderBarycentric;
		if (!graphicsCfg.enableBarycentricFS)
			MLOG_WARNING(u8"Barycentric FS requested but not supported");
		else
			MLOG_DEBUG(u8"Barycentric FS enabled!");
	}

	if (graphicsCfg.enableGeometryShader)
	{
		graphicsCfg.enableGeometryShader = enabledFeatures10.geometryShader = supportedFeatures.geometryShader;

		if (!graphicsCfg.enableGeometryShader)
			MLOG_WARNING(u8"Geometry Shader requested but not supported");
		else
			MLOG_DEBUG(u8"Geometry Shader enabled!");
	}

	// #ifdef ERMY_XR_OPENXR
	// 	enabledFeatures10.shaderStorageImageMultisample = supportedFeatures.shaderStorageImageMultisample;
	// #endif

	// #ifdef ERMY_OS_MACOS
	// 	device_extender.TryAddExtension(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
	// #endif

	enabledFeatures10.imageCubeArray = supportedFeatures.imageCubeArray;
	enabledFeatures10.fillModeNonSolid = supportedFeatures.fillModeNonSolid;
	enabledFeatures10.samplerAnisotropy = supportedFeatures.samplerAnisotropy;
	enabledFeatures10.largePoints = supportedFeatures.largePoints;

	VkDeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = gEnabledFeatures.BuildPChains();
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(requestedQueues.size());
	deviceCreateInfo.pQueueCreateInfos = requestedQueues.data();
	deviceCreateInfo.enabledLayerCount = 0; // deprecated
	deviceCreateInfo.ppEnabledLayerNames = nullptr;
	deviceCreateInfo.pEnabledFeatures = &enabledFeatures10;
	deviceCreateInfo.ppEnabledExtensionNames = device_extender.EnabledExtensions(); // deprecated
	deviceCreateInfo.enabledExtensionCount = device_extender.NumEnabledExtension();

	VK_CALL(vkCreateDevice(gVKPhysicalDevice, &deviceCreateInfo, gVKGlobalAllocationsCallbacks, &gVKDevice));

	LoadVkDeviceLevelFuncs(gVKDevice);

	vkGetDeviceQueue(gVKDevice, 0, 0, &gVKGraphicsQueue);

	// vk_utils::debug::SetName(gVKMainQueue, "Main Queue");

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = gVKPhysicalDevice;
	allocatorInfo.device = gVKDevice;
	allocatorInfo.instance = gVKInstance;

	if (gVKDeviceEnabledExtensions.KhrBufferDeviceAddress)
		allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	if (gVKDeviceEnabledExtensions.KhrDedicatedAllocation)
		allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;

	if (gVKDeviceEnabledExtensions.ExtMemoryPriority)
		allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;

	// VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT
	// VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT
	// VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT
	//
	// VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT
	// VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT
	// VMA_ALLOCATOR_CREATE_KHR_EXTERNAL_MEMORY_WIN32_BIT
	allocatorInfo.vulkanApiVersion = gPhysicalDeviceAPIVersion;

	VmaVulkanFunctions functions;

	functions.vkAllocateMemory = vkAllocateMemory;
	functions.vkBindBufferMemory = vkBindBufferMemory;

	functions.vkBindImageMemory = vkBindImageMemory;

	functions.vkCmdCopyBuffer = vkCmdCopyBuffer;
	functions.vkCreateBuffer = vkCreateBuffer;
	functions.vkCreateImage = vkCreateImage;
	functions.vkDestroyBuffer = vkDestroyBuffer;
	functions.vkDestroyImage = vkDestroyImage;
	functions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
	functions.vkFreeMemory = vkFreeMemory;
	functions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;

	functions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
	functions.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;
	functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
	functions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;

	functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	functions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
	functions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
	functions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
	functions.vkMapMemory = vkMapMemory;
	functions.vkUnmapMemory = vkUnmapMemory;

	functions.vkBindBufferMemory2KHR = vkBindBufferMemory2;
	functions.vkBindImageMemory2KHR = vkBindImageMemory2;
	functions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
	functions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
	functions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;

	allocatorInfo.pVulkanFunctions = &functions;

	vmaCreateAllocator(&allocatorInfo, &gVMA_Allocator);
}

void Device::Shutdown()
{
	MLOG_DEBUG(u8"Shutdown Device (Vulkan)");
}

void Device::Tick()
{
	// MLOG_DEBUG(u8"Tick Device (NULL)");
}

void Device::InitializeSwapchain()
{
	if (gSwapchain != nullptr)
	{
		MLOG_DEBUG(u8"Swapchain already initialized, skipping.");
		return;
	}

	gSwapchain = new Swapchain();
	gSwapchain->Initialize();
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

CommandPool Device::CreateCommandPool(QueueType queue_type)
{
	CommandPool command_pool;

	VkCommandPoolCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.queueFamilyIndex = 0; //todo
	create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	vkCreateCommandPool(gVKDevice, &create_info, nullptr, reinterpret_cast<VkCommandPool*>(&command_pool.nativePtr));
	return command_pool;
}

TimelineSemaphore Device::CreateTimelineSemaphore(mercury::u64 initial_value)
{
	TimelineSemaphore result;

	VkSemaphoreTypeCreateInfo timelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
		.pNext = nullptr,
		.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
		.initialValue = initial_value,
	};

	const VkSemaphoreCreateInfo semaphoreCreateInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = &timelineCreateInfo };
	VK_CALL(vkCreateSemaphore(gVKDevice, &semaphoreCreateInfo, nullptr, reinterpret_cast<VkSemaphore*>(&result.nativePtr)));

	return result;
}

void Device::WaitIdle()
{
	vkDeviceWaitIdle(gVKDevice);
}

void Device::WaitQueueIdle(QueueType queue_type)
{
	vkQueueWaitIdle(gVKGraphicsQueue);
}

void Device::SetDebugName(const char* utf8_name)
{
	if (gVKDevice != VK_NULL_HANDLE)
	{
		vk_utils::debug::SetName(gVKDevice, utf8_name);
	}
	else
	{
		MLOG_WARNING(u8"Cannot set debug name for device, gVKDevice is null.");
	}
}

#endif