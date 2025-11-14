#include "ll/graphics.h"

#if defined(MERCURY_LL_GRAPHICS_VULKAN)

using namespace mercury;
using namespace mercury::ll::graphics;

bool mercury::ll::graphics::IsYFlipped()
{
	return false;
}

#include "vk_graphics.h"
#include "vk_swapchain.h"
#include "vk_device.h"
#include "vk_utils.h"

#include "../../../imgui/imgui_impl.h"

VkDevice gVKDevice = VK_NULL_HANDLE;
VkQueue gVKGraphicsQueue = VK_NULL_HANDLE;
VkQueue gVKTransferQueue = VK_NULL_HANDLE;
VkQueue gVKComputeQueue = VK_NULL_HANDLE;
VmaAllocator gVMA_Allocator = nullptr;
VkDescriptorPool gVKGlobalDescriptorPool = VK_NULL_HANDLE;

DeviceEnabledExtensions gVKDeviceEnabledExtensions;
std::vector<PipelineObjects> gAllPSOs;
std::vector<ShaderModuleCached> gAllShaderModules;
std::vector<VkDescriptorSetLayout> gAllDSLayouts;
std::vector<VkDescriptorSet> gAllDescriptorSets;

VkSampler gVKDefaultLinearSampler = VK_NULL_HANDLE;
VkSampler gVKDefaultNearestSampler = VK_NULL_HANDLE;
VkSampler gVKDefaultTrilinearSampler = VK_NULL_HANDLE;
VkSampler gVKDefaultAnisotropicSampler = VK_NULL_HANDLE;

struct BufferInfo
{
	VmaAllocation allocation = nullptr;
	size_t size = 0;
	void* persistentMappedPtr = nullptr;
};

std::vector<VkBuffer> gAllBuffers;
std::vector<BufferInfo> gAllBufferMetas;

struct TextureInfo
{
	VmaAllocation allocation = nullptr;
	VmaAllocationInfo allocationInfo = {};
	VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImage image = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
};

std::vector<TextureInfo> gAllTextures;

struct OneTimeSubmitContext
{
	VkCommandPool commandPool = VK_NULL_HANDLE;
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkFence fence = VK_NULL_HANDLE;
	bool inUse = false;
	std::function<void()> onFinish = nullptr;
};

std::vector<OneTimeSubmitContext> gOneTimeSubmitContexts;

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
	features11.shaderDrawParameters = true;
	
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



VkSamplerCreateInfo createDefaultSampler()
{
	VkSamplerCreateInfo sampler_info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.maxAnisotropy = 1.0f;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = VK_LOD_CLAMP_NONE;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	return sampler_info;
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
	deviceCreateInfo.ppEnabledLayerNames = nullptr; // deprecated
	deviceCreateInfo.pEnabledFeatures = &enabledFeatures10;
	deviceCreateInfo.ppEnabledExtensionNames = device_extender.EnabledExtensions();
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

	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 10000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10000 }
	};

	VkDescriptorPoolCreateInfo dp_create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,nullptr };
	dp_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	dp_create_info.maxSets = 1000;
	dp_create_info.pPoolSizes = pool_sizes;
	dp_create_info.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]);

	vkCreateDescriptorPool(gVKDevice, &dp_create_info, nullptr, &gVKGlobalDescriptorPool);

	{
		VkSamplerCreateInfo sampler_info = createDefaultSampler();
		sampler_info.anisotropyEnable = VK_TRUE;
		sampler_info.maxAnisotropy = 16.0f;
		vkCreateSampler(gVKDevice, &sampler_info, gVKGlobalAllocationsCallbacks, &gVKDefaultAnisotropicSampler);
	}

	{
		VkSamplerCreateInfo sampler_info = createDefaultSampler();
		vkCreateSampler(gVKDevice, &sampler_info, gVKGlobalAllocationsCallbacks, &gVKDefaultTrilinearSampler);
	}

	{
		VkSamplerCreateInfo sampler_info = createDefaultSampler();
		sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		vkCreateSampler(gVKDevice, &sampler_info, gVKGlobalAllocationsCallbacks, &gVKDefaultLinearSampler);
	}

	{
		VkSamplerCreateInfo sampler_info = createDefaultSampler();
		sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		sampler_info.magFilter = VK_FILTER_NEAREST;
		sampler_info.minFilter = VK_FILTER_NEAREST;
		vkCreateSampler(gVKDevice, &sampler_info, gVKGlobalAllocationsCallbacks, &gVKDefaultNearestSampler);
	}

	gOneTimeSubmitContexts.resize(64);
	for(int i=0;i<gOneTimeSubmitContexts.size();i++)
	{
		auto& context = gOneTimeSubmitContexts[i];

		{
			VkCommandPoolCreateInfo create_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
			create_info.queueFamilyIndex = 0; //todo			
			vkCreateCommandPool(gVKDevice, &create_info, nullptr, reinterpret_cast<VkCommandPool*>(&context.commandPool));
		}

		{
			VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
			alloc_info.commandPool = context.commandPool;
			alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			alloc_info.commandBufferCount = 1;

			vkAllocateCommandBuffers(gVKDevice, &alloc_info, &context.commandBuffer);
		}

		{
			VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
			fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			vkCreateFence(gVKDevice, &fence_info, nullptr, &context.fence);
		}
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

static void mercury_imgui_check_vk_result(VkResult err)
{
}

void Device::ImguiInitialize()
{
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = gVKInstance;
	init_info.PhysicalDevice = gVKPhysicalDevice;
	init_info.Device = gVKDevice;
	init_info.QueueFamily = 0;
	init_info.Queue = gVKGraphicsQueue;
	init_info.PipelineCache = VK_NULL_HANDLE; //TODO: implement pipeline cache
	init_info.Allocator = nullptr;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.CheckVkResultFn = mercury_imgui_check_vk_result;
	init_info.PipelineInfoMain.MSAASamples = gVKSurfaceSamples;
	init_info.PipelineInfoMain.RenderPass = gVKFinalRenderPass;
	init_info.MinAllocationSize = 1024 * 1024;

	init_info.UseDynamicRendering = gVKConfig.useDynamicRendering;
	if (gVKConfig.useDynamicRendering)
	{
		VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
		pipeline_rendering_create_info.colorAttachmentCount = 1;
		pipeline_rendering_create_info.pColorAttachmentFormats = &gVKSurfaceFormat;
		init_info.PipelineInfoMain.PipelineRenderingCreateInfo = pipeline_rendering_create_info;
	}
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo dp_create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,nullptr };
	dp_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	dp_create_info.maxSets = 1000;
	dp_create_info.pPoolSizes = pool_sizes;
	dp_create_info.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]);

	vkCreateDescriptorPool(gVKDevice, &dp_create_info, nullptr, &init_info.DescriptorPool);

	ImGui_ImplVulkan_Init(&init_info);
	ImguiRegenerateFontAtlas();
}

void Device::ImguiRegenerateFontAtlas()
{
	//ImGui_ImplVulkan_CreateFontsTexture();
}

void Device::ImguiNewFrame()
{
	ImGui_ImplVulkan_NewFrame();
}

void Device::ImguiShutdown()
{
	ImGui_ImplVulkan_Shutdown();
}

void _createShaderModuleCache(const ShaderBytecodeView& bytecode, ShaderModuleCached& out)
{
	VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	createInfo.codeSize = bytecode.size;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data);
	VK_CALL(vkCreateShaderModule(gVKDevice, &createInfo, gVKGlobalAllocationsCallbacks, &out.module));
}

ShaderHandle Device::CreateShaderModule(const ShaderBytecodeView& bytecode)
{
	ShaderHandle result;
	auto& psoOut = gAllShaderModules.emplace_back();
	result.handle = static_cast<u32>(gAllShaderModules.size() - 1);
	_createShaderModuleCache(bytecode, psoOut);
	return result;
}

void Device::UpdateShaderModule(ShaderHandle shaderModuleID, const ShaderBytecodeView& bytecode)
{
	vkDestroyShaderModule(gVKDevice, gAllShaderModules[shaderModuleID.handle].module, gVKGlobalAllocationsCallbacks);
	_createShaderModuleCache(bytecode, gAllShaderModules[shaderModuleID.handle]);
}

void Device::DestroyShaderModule(ShaderHandle shaderModuleID)
{
	vkDestroyShaderModule(gVKDevice, gAllShaderModules[shaderModuleID.handle].module, gVKGlobalAllocationsCallbacks);
}

VkPrimitiveTopology fromMercuryPrimitiveTopology(PrimitiveTopology topology)
{
	switch (topology)
	{
	case PrimitiveTopology::PointList:
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case PrimitiveTopology::LineList:
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case PrimitiveTopology::LineStrip:
		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case PrimitiveTopology::TriangleList:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	case PrimitiveTopology::TriangleStrip:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case PrimitiveTopology::PatchList:
		return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	default:
		MERCURY_ASSERT(false && "Unknown Primitive Topology");
		return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	}
}

void _createGraphicsPSO(const RasterizePipelineDescriptor& desc, PipelineObjects& out)
{
	VkPipelineLayoutCreateInfo layoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};	

	std::vector<VkPushConstantRange> pcRanges;


	if(desc.pushConstantSize > 0)
	{
		VkPushConstantRange& pcr = pcRanges.emplace_back();
		pcr.offset = 0;
		pcr.size = desc.pushConstantSize;
		pcr.stageFlags = VK_SHADER_STAGE_ALL; // TODO: specify stages		
	}

	std::vector<VkDescriptorSetLayout> setLayouts;

	for (int i = 0; i < 4; ++i)
	{
		IF_UNLIKELY (desc.bindingSetLayouts[i].allSlots.empty())
			continue;

		auto outLayout = gDevice->CreateParameterBlockLayout(desc.bindingSetLayouts[i], i); // ensure layout is created and cached

		IF_LIKELY (outLayout.isValid())
		{
			setLayouts.push_back(gAllDSLayouts[outLayout.handle]);
		}
	}


	layoutCreateInfo.pushConstantRangeCount = static_cast<u32>(pcRanges.size());
	layoutCreateInfo.pPushConstantRanges = pcRanges.data();

	layoutCreateInfo.setLayoutCount = static_cast<u32>(setLayouts.size());
	layoutCreateInfo.pSetLayouts = setLayouts.data();

	vkCreatePipelineLayout(gVKDevice, &layoutCreateInfo, gVKGlobalAllocationsCallbacks, &out.pipelineLayout);


	VkGraphicsPipelineCreateInfo psoCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	psoCreateInfo.layout = out.pipelineLayout;

	psoCreateInfo.renderPass = gVKFinalRenderPass;
	psoCreateInfo.subpass = 0;

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;	
	VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	VkPipelineTessellationStateCreateInfo tessellationState = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };

	if(desc.vertexShader.isValid())
	{
		auto& shaderModule = gAllShaderModules[desc.vertexShader.handle];
		VkPipelineShaderStageCreateInfo stageInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
		stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		stageInfo.module = shaderModule.module;
		stageInfo.pName = "main";
		shaderStages.push_back(stageInfo);
	}

	if(desc.fragmentShader.isValid())
	{
		auto& shaderModule = gAllShaderModules[desc.fragmentShader.handle];
		VkPipelineShaderStageCreateInfo stageInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
		stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stageInfo.module = shaderModule.module;
		stageInfo.pName = "main";
		shaderStages.push_back(stageInfo);
	}

	inputAssemblyState.topology = fromMercuryPrimitiveTopology(desc.primitiveTopology);

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(gVKSurfaceCaps.currentExtent.width);
	viewport.height = static_cast<float>(gVKSurfaceCaps.currentExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	viewportState.pViewports = &viewport;
	viewportState.viewportCount = 1;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { gVKSurfaceCaps.currentExtent.width, gVKSurfaceCaps.currentExtent.height };
	viewportState.pScissors = &scissor;
	viewportState.scissorCount = 1;

	rasterizationState.lineWidth = 1.0f;
	rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;

	multisampleState.rasterizationSamples = gVKSurfaceSamples;

	VkPipelineColorBlendAttachmentState attachment = {};
	attachment.blendEnable = false;
	attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &attachment;

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		//VK_DYNAMIC_STATE_LINE_WIDTH
	};

	dynamicState.pDynamicStates = dynamicStates.data();
	dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());

	
	psoCreateInfo.pStages = shaderStages.data();
	psoCreateInfo.stageCount = static_cast<u32>(shaderStages.size());

	psoCreateInfo.pVertexInputState = &vertexInputState; // TODO: vertex input state
	psoCreateInfo.pInputAssemblyState = &inputAssemblyState; // TODO: input assembly state
	psoCreateInfo.pTessellationState = nullptr; // TODO: tessellation state
	psoCreateInfo.pViewportState = &viewportState; // TODO: viewport state
	psoCreateInfo.pRasterizationState = &rasterizationState; // TODO: rasterization state
	psoCreateInfo.pMultisampleState = &multisampleState; // TODO: multisample state
	psoCreateInfo.pDepthStencilState = &depthStencilState; // TODO: depth stencil state
	psoCreateInfo.pColorBlendState =  &colorBlendState; // TODO: color blend state
	psoCreateInfo.pDynamicState = &dynamicState; // TODO: dynamic state
	
	VK_CALL(vkCreateGraphicsPipelines(gVKDevice, VK_NULL_HANDLE, 1, &psoCreateInfo, gVKGlobalAllocationsCallbacks, &out.pipeline));

	//desc.fragmentShader.size
	//psoCreateInfo.stageCount = 0; // TODO: shader stages
}

PsoHandle Device::CreateRasterizePipeline(const RasterizePipelineDescriptor& desc)
{	
	PsoHandle result;

	auto& psoOut = gAllPSOs.emplace_back();
	result.handle = static_cast<u32>(gAllPSOs.size() - 1);

	_createGraphicsPSO(desc, psoOut);
	
	return result;
}

void Device::DestroyRasterizePipeline(PsoHandle psoID)
{
	//TODO: Think about delayed descruction while GPU is using old PSO
	vkDestroyPipeline(gVKDevice, gAllPSOs[psoID.handle].pipeline, gVKGlobalAllocationsCallbacks);
	vkDestroyPipelineLayout(gVKDevice, gAllPSOs[psoID.handle].pipelineLayout, gVKGlobalAllocationsCallbacks);
}

void Device::UpdatePipelineState(PsoHandle psoID, const RasterizePipelineDescriptor& desc)
{
	VkPipeline oldPipeline = gAllPSOs[psoID.handle].pipeline;
	VkPipelineLayout oldLayout = gAllPSOs[psoID.handle].pipelineLayout;

	_createGraphicsPSO(desc, gAllPSOs[psoID.handle]);

	//TODO: Think about delayed descruction while GPU is using old PSO
	//vkDestroyPipeline(gVKDevice, oldPipeline, gVKGlobalAllocationsCallbacks);
	//vkDestroyPipelineLayout(gVKDevice, oldLayout, gVKGlobalAllocationsCallbacks);
}

void CommandList::RenderImgui()
{	
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VkCommandBuffer>(nativePtr));
}

void CommandList::SetPSO(PsoHandle psoID)
{
	vkCmdBindPipeline(static_cast<VkCommandBuffer>(nativePtr), VK_PIPELINE_BIND_POINT_GRAPHICS, gAllPSOs[psoID.handle].pipeline);

	currentPsoID = psoID;
	currentPSOnativePtr = gAllPSOs[psoID.handle].pipeline;
	currentPSOLayoutNativePtr = gAllPSOs[psoID.handle].pipelineLayout;
}

void CommandList::Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
{
	vkCmdDraw(static_cast<VkCommandBuffer>(nativePtr), vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
	VkViewport viewport;
	viewport.x = x;
	viewport.y = y;
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;

	vkCmdSetViewport(static_cast<VkCommandBuffer>(nativePtr), 0, 1, &viewport);
}

void CommandList::SetScissor(i32 x, i32 y, u32 width, u32 height)
{
	VkRect2D scissor;
	scissor.offset = { x, y };
	scissor.extent = { width, height };

	vkCmdSetScissor(static_cast<VkCommandBuffer>(nativePtr), 0, 1, &scissor);
}

void CommandList::PushConstants(const void* data, size_t size)
{
	auto cmdBuff = static_cast<VkCommandBuffer>(nativePtr);

	vkCmdPushConstants(
		cmdBuff,
		/* Assuming pipeline layout is available as 'pipelineLayout' */
		static_cast<VkPipelineLayout>(currentPSOLayoutNativePtr),
		VK_SHADER_STAGE_ALL, // or specify particular stages
		0,
		static_cast<uint32_t>(size),
		data
	);
}


void Device::DestroyBuffer(BufferHandle bufferID)
{
	// Implementation for destroying a buffer
}

void Device::UpdateBuffer(BufferHandle bufferID, const void* data, size_t size, size_t offset)
{
	const u32 idx = bufferID.handle;
	if (idx >= gAllBufferMetas.size())
	{
		MLOG_WARNING(u8"UpdateBuffer: invalid buffer handle (%u)", idx);
		return;
	}
	auto& meta = gAllBufferMetas[idx];

	if ((offset + size) > meta.size)
	{
		MLOG_WARNING(u8"UpdateBuffer: write out of bounds (offset=%zu, size=%zu, capacity=%zu). Clamping.",
			offset, size, meta.size);
		if (offset >= meta.size) return;
		size = meta.size - offset;
	}

	// Ensure persistent mapping exists (should be already mapped via VMA_ALLOCATION_CREATE_MAPPED_BIT).
	if (meta.persistentMappedPtr == nullptr)
	{
		void* mapped = nullptr;
		VK_CALL(vmaMapMemory(gVMA_Allocator, meta.allocation, &mapped));
		meta.persistentMappedPtr = mapped;
	}

	std::memcpy(static_cast<char*>(meta.persistentMappedPtr) + offset, data, size);

	// Flush the written range (no-op if memory is HOST_COHERENT).
	vmaFlushAllocation(gVMA_Allocator, meta.allocation, static_cast<VkDeviceSize>(offset), static_cast<VkDeviceSize>(size));
}

BufferHandle Device::CreateBuffer(const BufferDescriptor& desc)
{
	auto size = desc.size;

	VkBufferCreateInfo bufCI{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufCI.size = static_cast<VkDeviceSize>(size);
	bufCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT; // uniform buffer usage
	bufCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocCI{};
	// Persistently map the buffer for CPU updates.
	allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	// We need CPU access.
	allocCI.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	// Prefer device-local if a host-visible device-local type exists (BAR/reBAR); also prefer coherent if available.
	allocCI.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	// CPU-to-GPU access pattern (host visible, optimal for uploads/UBOs).
	allocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	VkBuffer vkBuf = VK_NULL_HANDLE;
	VmaAllocation allocation = nullptr;
	VmaAllocationInfo allocInfo{};
	VK_CALL(vmaCreateBuffer(gVMA_Allocator, &bufCI, &allocCI, &vkBuf, &allocation, &allocInfo));

	BufferInfo meta{};
	meta.allocation = allocation;
	meta.size = size;
	meta.persistentMappedPtr = allocInfo.pMappedData; // persistent map

	if (desc.initialData != nullptr)
	{
		// Copy initial data into the buffer.
		std::memcpy(meta.persistentMappedPtr, desc.initialData, size);
		// Flush the written range (no-op if memory is HOST_COHERENT).
		vmaFlushAllocation(gVMA_Allocator, meta.allocation, 0, static_cast<VkDeviceSize>(size));
	}

	gAllBuffers.push_back(vkBuf);
	gAllBufferMetas.push_back(meta);

	BufferHandle h;
	h.handle = static_cast<u32>(gAllBuffers.size() - 1);
	return h;
}

ParameterBlockLayoutHandle Device::CreateParameterBlockLayout(const BindingSetLayoutDescriptor& layoutDesc, int setIndex)
{
	if (layoutDesc.allSlots.empty())
	{
		return ParameterBlockLayoutHandle{ ParameterBlockLayoutHandle::InvalidValue };
	}

	VkDescriptorSetLayout& outLayout = gAllDSLayouts.emplace_back();;

	VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	for (int s = 0; s < layoutDesc.allSlots.size(); ++s)
	{
		VkDescriptorSetLayoutBinding bindingDesc = {};
		const auto& slot = layoutDesc.allSlots[s];

		if (slot.resourceType == ShaderResourceType::UniformBuffer)
		{
			bindingDesc.binding = s;
			bindingDesc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; //TODO: other types
			bindingDesc.descriptorCount = 1;
			bindingDesc.stageFlags = VK_SHADER_STAGE_ALL; // TODO: specify stages
			bindingDesc.pImmutableSamplers = nullptr;
		}

		if (slot.resourceType == ShaderResourceType::SampledImage2D)
		{
			bindingDesc.binding = s;
			bindingDesc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; //TODO: other types
			bindingDesc.descriptorCount = 1;
			bindingDesc.stageFlags = VK_SHADER_STAGE_ALL; // TODO: specify stages
			bindingDesc.pImmutableSamplers = &gVKDefaultLinearSampler;
		}

		bindings.push_back(bindingDesc);
	}

	createInfo.bindingCount = static_cast<u32>(bindings.size());
	createInfo.pBindings = bindings.data();

	if (createInfo.bindingCount > 0)
	{
		vkCreateDescriptorSetLayout(gVKDevice, &createInfo, gVKGlobalAllocationsCallbacks, &outLayout);
	}

	return ParameterBlockLayoutHandle{static_cast<u32>(gAllDSLayouts.size() - 1)};
}

void Device::DestroyParameterBlockLayout(ParameterBlockLayoutHandle layoutID)
{
	//vkAllocateDescriptorSets()
	// Implementation for destroying a parameter block layout
}

void CommandList::SetParameterBlock(u8 setIndex, ParameterBlockHandle parameterBlockID)
{
	auto cmdBuff = static_cast<VkCommandBuffer>(nativePtr);

	vkCmdBindDescriptorSets(
		cmdBuff,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		static_cast<VkPipelineLayout>(currentPSOLayoutNativePtr),
		setIndex,
		1,
		&gAllDescriptorSets[parameterBlockID.handle],
		0,
		nullptr
	);
}

ParameterBlockHandle Device::CreateParameterBlock(const ParameterBlockLayoutHandle& layoutID)
{
	auto& ds = gAllDescriptorSets.emplace_back();

	VkDescriptorSetAllocateInfo cinfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	cinfo.descriptorPool = gVKGlobalDescriptorPool;
	cinfo.descriptorSetCount = 1;
	cinfo.pSetLayouts = &gAllDSLayouts[layoutID.handle];

	vkAllocateDescriptorSets(gVKDevice, &cinfo, &ds);

	return ParameterBlockHandle{ static_cast<u32>(gAllDescriptorSets.size() - 1) };
}

void Device::UpdateParameterBlock(ParameterBlockHandle parameterBlockID, const ParameterBlockDescriptor& pbDesc)
{
	std::vector<VkWriteDescriptorSet> writes;
	std::vector<VkDescriptorBufferInfo> bufferInfos;
	std::vector<VkDescriptorImageInfo>  imageInfos;

	u32 slotIndex = 0;
	writes.reserve(pbDesc.resources.size());
	bufferInfos.reserve(pbDesc.resources.size());
	imageInfos.reserve(pbDesc.resources.size());

	for (const auto& res : pbDesc.resources)
	{
		std::visit([&](auto&& arg)
			{
				using T = std::decay_t<decltype(arg)>;

				if constexpr (std::is_same_v<T, ParameterResourceBuffer>)
				{
					VkDescriptorBufferInfo bi{};
					bi.buffer = gAllBuffers[arg.buffer.handle];
					bi.offset = static_cast<VkDeviceSize>(arg.offset);
					bi.range = (arg.size == SIZE_MAX) ? VK_WHOLE_SIZE : static_cast<VkDeviceSize>(arg.size);

					bufferInfos.push_back(bi);

					VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
					w.dstSet = gAllDescriptorSets[parameterBlockID.handle];
					w.dstBinding = slotIndex;
					w.descriptorCount = 1;
					w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					w.pBufferInfo = &bufferInfos.back();

					writes.push_back(w);
				}
				else if constexpr (std::is_same_v<T, ParameterResourceTexture>)
				{
					VkDescriptorImageInfo ii{};
					ii.imageView = gAllTextures[arg.texture.handle].imageView;
					ii.sampler = VK_NULL_HANDLE;
					ii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfos.push_back(ii);

					VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
					w.dstSet = gAllDescriptorSets[parameterBlockID.handle];
					w.dstBinding = slotIndex;
					w.descriptorCount = 1;
					w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					w.pImageInfo = &imageInfos.back();

					writes.push_back(w);

					MLOG_WARNING(u8"ParameterResourceTexture not yet implemented in UpdateParameterBlock");
				}
				else if constexpr (std::is_same_v<T, ParameterResourceRWImage>)
				{
					// TODO: Fill VkDescriptorImageInfo for storage image
					MLOG_WARNING(u8"ParameterResourceRWImage not yet implemented in UpdateParameterBlock");
				}
				else if constexpr (std::is_same_v<T, ParameterResourceEmpty>)
				{
					// Intentionally empty slot – skip writing
				}
			}, res);

		slotIndex++;
	}

	if (!writes.empty())
	{
		vkUpdateDescriptorSets(gVKDevice, static_cast<u32>(writes.size()), writes.data(), 0, nullptr);
	}
}

void Device::DestroyParameterBlock(ParameterBlockHandle parameterBlockID)
{

}

TextureHandle Device::CreateTexture(const TextureDescriptor& desc)
{
	TextureHandle result;
	result.handle = static_cast<u32>(gAllTextures.size());

	auto& texOut = gAllTextures.emplace_back();

	VmaAllocationCreateInfo vmaAllocCI{};
	vmaAllocCI.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VkImageCreateInfo imageCI{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCI.extent.width = static_cast<u32>(desc.width);
	imageCI.extent.height = static_cast<u32>(desc.height);
	imageCI.extent.depth = 1;
	imageCI.mipLevels = static_cast<u32>(desc.mipLevels);
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VK_CALL(vmaCreateImage(gVMA_Allocator, &imageCI, &vmaAllocCI, &texOut.image, &texOut.allocation, &texOut.allocationInfo));

	VkImageViewCreateInfo imageViewCI{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};

	imageViewCI.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageViewCI.image = texOut.image;
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCI.subresourceRange.baseMipLevel = 0;
	imageViewCI.subresourceRange.levelCount = static_cast<u32>(desc.mipLevels);
	imageViewCI.subresourceRange.baseArrayLayer = 0;
	imageViewCI.subresourceRange.layerCount = 1;

	VK_CALL(vkCreateImageView(gVKDevice, &imageViewCI, gVKGlobalAllocationsCallbacks, &texOut.imageView));
	
	if (desc.initialData != nullptr)
	{
		BufferDescriptor stagingDesc{};
		stagingDesc.size = desc.width * desc.height * 4; // TODO: calculate proper size based on format
		stagingDesc.initialData = desc.initialData;

		BufferHandle stagingBuffer = gDevice->CreateBuffer(stagingDesc);

		gDevice->SubmitOneTimeCommandsList(
			[=](CommandList& cmdList)
			{
				// Transition image layout to TRANSFER_DST_OPTIMAL
				VkImageMemoryBarrier barrierToTransfer{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
				barrierToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				barrierToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrierToTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrierToTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrierToTransfer.image = texOut.image;
				barrierToTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrierToTransfer.subresourceRange.baseMipLevel = 0;
				barrierToTransfer.subresourceRange.levelCount = static_cast<u32>(desc.mipLevels);
				barrierToTransfer.subresourceRange.baseArrayLayer = 0;
				barrierToTransfer.subresourceRange.layerCount = 1;
				barrierToTransfer.srcAccessMask = 0;
				barrierToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				vkCmdPipelineBarrier(
					static_cast<VkCommandBuffer>(cmdList.nativePtr),
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrierToTransfer);
				// Copy buffer to image
				VkBufferImageCopy copyRegion{};
				copyRegion.bufferOffset = 0;
				copyRegion.bufferRowLength = 0;
				copyRegion.bufferImageHeight = 0;
				copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.imageSubresource.mipLevel = 0;
				copyRegion.imageSubresource.baseArrayLayer = 0;
				copyRegion.imageSubresource.layerCount = 1;
				copyRegion.imageOffset = { 0, 0, 0 };
				copyRegion.imageExtent = { static_cast<u32>(desc.width), static_cast<u32>(desc.height), 1 };

				vkCmdCopyBufferToImage(
					static_cast<VkCommandBuffer>(cmdList.nativePtr),
					gAllBuffers[stagingBuffer.handle],
					texOut.image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&copyRegion);

				// Transition image layout to SHADER_READ_ONLY_OPTIMAL
				VkImageMemoryBarrier barrierToShaderRead{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
				barrierToShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrierToShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrierToShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrierToShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrierToShaderRead.image = texOut.image;
				barrierToShaderRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrierToShaderRead.subresourceRange.baseMipLevel = 0;
				barrierToShaderRead.subresourceRange.levelCount = static_cast<u32>(desc.mipLevels);
				barrierToShaderRead.subresourceRange.baseArrayLayer = 0;
				barrierToShaderRead.subresourceRange.layerCount = 1;
				barrierToShaderRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrierToShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				vkCmdPipelineBarrier(
					static_cast<VkCommandBuffer>(cmdList.nativePtr),
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrierToShaderRead);
			});
	}

	return result;
}

void Device::DestroyTexture(TextureHandle textureID)
{

}

void Device::UpdateTexture(TextureHandle textureID, const void* data, size_t size, size_t offset)
{
	//TODO: Implement texture update
}

void Device::UpdateSubregionTexture(
	TextureHandle textureID,
	size_t x,
	size_t y,
	size_t z,
	size_t width,
	size_t height,
	size_t depth,
	const void* data,
	size_t dataSize)
{
	//TODO: Implement subregion texture update
}

u64 TextureHandle::CreateImguiTextureOpaqueHandle() const
{
	const auto& tex_data = &gAllTextures[handle];
	VkDescriptorSet ds = ImGui_ImplVulkan_AddTexture(gVKDefaultLinearSampler, tex_data->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	return (u64)(intptr_t)ds;
}

void Device::SubmitOneTimeCommandsList(std::function<void(CommandList& cmdList)> recordCommands, std::function<void()> onFinish)
{
	//get ready one time submit commands
	while (true)
	{
		for (int i = 0; i < gOneTimeSubmitContexts.size(); i++)
		{
			auto& cc = gOneTimeSubmitContexts[i];
			if (vkGetFenceStatus(gVKDevice, cc.fence) == VK_SUCCESS)
			{
				if (cc.inUse)
				{
					if (cc.onFinish)
						cc.onFinish();
				}

				cc.inUse = true;

				vkResetCommandPool(gVKDevice, cc.commandPool, 0);
				VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				vkBeginCommandBuffer(cc.commandBuffer, &beginInfo);
				CommandList cmdList;
				cmdList.nativePtr = cc.commandBuffer;
				recordCommands(cmdList);
				vkEndCommandBuffer(cc.commandBuffer);
				VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &cc.commandBuffer;
				vkResetFences(gVKDevice, 1, &cc.fence);
				vkQueueSubmit(gVKGraphicsQueue, 1, &submitInfo, cc.fence);
				cc.onFinish = onFinish;
				return;
			}
		}

		ll::os::gOS->Sleep(1);
	}
}

#endif