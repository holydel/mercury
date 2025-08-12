#include "vk_graphics.h"

#ifdef MERCURY_LL_GRAPHICS_VULKAN

#include "mercury_log.h"
#include "mercury_memory.h"
#include "mercury_application.h"
#include <sstream>
#include "../ll_graphics.h"
#include "vk_swapchain.h"

using namespace mercury;
using namespace mercury::ll::graphics;

mercury::u32 gPhysicalDeviceAPIVersion = 0;
VkInstance gVKInstance = VK_NULL_HANDLE;
VkAllocationCallbacks* gVKGlobalAllocationsCallbacks = nullptr;
VkDebugUtilsMessengerEXT gVKDebugMessenger = VK_NULL_HANDLE;

std::vector<VkPhysicalDevice> gAllPhysicalDevices;

mercury::memory::ReservedAllocator* memory::gGraphicsMemoryAllocator = nullptr;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{

    if (pCallbackData->messageIdNumber == 0)
    {
        if (strcmp(pCallbackData->pMessageIdName, "Loader Message") == 0)
        {
            logging::write_message(logging::Severity::Info, u8"VULKAN_LOADER", (const char8_t *)pCallbackData->pMessage);
            return VK_FALSE;
        }
    }
    else
    {
        if (pCallbackData->messageIdNumber == 0x7f1922d7)
        {
            logging::write_message(logging::Severity::Info,  u8"VULKAN", (const char8_t *)pCallbackData->pMessage);
            return VK_FALSE;
        }
    }

    auto severity = logging::Severity::Info;
    
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        severity = logging::Severity::Info;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        severity = logging::Severity::Warning;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        severity = logging::Severity::Error;
        break;
    }

    logging::write_message(severity, u8"VULKAN", (const char8_t *)pCallbackData->pMessage);

    return VK_FALSE;
}

const char* SystemAllocationScopeToString(VkSystemAllocationScope scope)
{
    switch (scope)
    {
    case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND: return "Command";
    case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT: return "Object";
    case VK_SYSTEM_ALLOCATION_SCOPE_CACHE: return "Cache";
    case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE: return "Device";
    case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE: return "Instance";
    default: return "Unknown";
    }
}

struct AllocationsCallbacks : public VkAllocationCallbacks
{
    AllocationsCallbacks()
    {
        pfnAllocation = [](void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) -> void* {
            return memory::gGraphicsMemoryAllocator->Allocate(size);
        };
        pfnReallocation = [](void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) -> void* {
           // MLOG_DEBUG(u8"Vulkan reallocation: %zu bytes scope: %s", size, SystemAllocationScopeToString(allocationScope));
            return memory::gGraphicsMemoryAllocator->ReAllocate(pOriginal, size);
        };
        pfnFree = [](void* pUserData, void* pMemory) {
            memory::gGraphicsMemoryAllocator->Deallocate(pMemory);
        };
        pfnInternalAllocation = [](void* pUserData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope allocationScope) {
            MLOG_DEBUG(u8"Vulkan internal allocation: %zu bytes scope: %s", size, SystemAllocationScopeToString(allocationScope));
        };
        pfnInternalFree = [](void* pUserData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope allocationScope) {
            MLOG_DEBUG(u8"Vulkan internal free: %zu bytes scope: %s", size, SystemAllocationScopeToString(allocationScope));
        };
    }
};

struct AllocationsCallbacksMalloc : public VkAllocationCallbacks
{
    AllocationsCallbacksMalloc()
    {
        pfnAllocation = [](void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) -> void* {
            return malloc(size);
        };
        pfnReallocation = [](void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) -> void* {
           // MLOG_DEBUG(u8"Vulkan reallocation: %zu bytes scope: %s", size, SystemAllocationScopeToString(allocationScope));
            return realloc(pOriginal, size);
        };
        pfnFree = [](void* pUserData, void* pMemory) {
            free(pMemory);
        };
        pfnInternalAllocation = [](void* pUserData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope allocationScope) {
            MLOG_DEBUG(u8"Vulkan internal allocation: %zu bytes scope: %s", size, SystemAllocationScopeToString(allocationScope));
        };
        pfnInternalFree = [](void* pUserData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope allocationScope) {
            MLOG_DEBUG(u8"Vulkan internal free: %zu bytes scope: %s", size, SystemAllocationScopeToString(allocationScope));
        };
    }
};

struct ValidationSettings
{
    VkBool32 truePtr{VK_TRUE};
    VkBool32 falsePtr{VK_FALSE};

    std::vector<const char *> debug_action{"VK_DBG_LAYER_ACTION_DEBUG_OUTPUT", "VK_DBG_LAYER_ACTION_LOG_MSG"}; // "VK_DBG_LAYER_ACTION_DEBUG_OUTPUT", "VK_DBG_LAYER_ACTION_BREAK"
    std::vector<const char *> report_flags{"warn", "perf", "error"};                                           //"info", "warn", "perf", "error", "debug"

    uint32_t setting_duplicate_message_limit = 32;

    std::vector<unsigned int> muteVUIds = {
        0xdd73dbcf, // Validation Error: [ VUID-VkPhysicalDeviceProperties2-pNext-pNext ] | MessageID = 0xdd73dbcf | vkGetPhysicalDeviceProperties2(): pProperties->pNext chain includes a structure with unknown VkStructureType
        0x916108d1, // perf BestPractices-NVIDIA-ClearColor-NotCompressed
        0x675dc32e, // perf BestPractices-specialuse-extension vkCreateInstance(): Attempting to enable extension VK_EXT_debug_utils, but this ext
        0xa96ad8,   // perf  BestPractices-NVIDIA-BindMemory-NoPriorityUse vkSetDeviceMemoryPriorityEXT to provide the OS with information on which allocations should stay in memory and which should be demoted first when video memory is limited. The highest priority should be given to GPU-written resources like color attachments, depth attachments, storage images, and buffers written from the GPU. Validation Performance Warning : [BestPractices - NVIDIA - BindMemory - NoPriority] | MessageID = 0xa96ad8 | vkBindImageMemory() : [NVIDIA] Use vkSetDeviceMemoryPriorityEXT to provide the OS with information on which allocations should stay in memory and which should be demoted first when video memory is limited.The highest priority should be given to GPU - written resources like color attachments, depth attachments, storage images, and buffers written from the GPU.
        0xc91ae640, // perf  BestPractices-vkEndCommandBuffer-VtxIndexOutOfBounds vkEndCommandBuffer(): Vertex buffers was bound to VkCommandBuffer 0x123e73703d50[] but no draws had a pipeline that used the vertex buffer.
        0xc714b932, // perf [ BestPractices-NVIDIA-AllocateMemory-ReuseAllocations ] vkAllocateMemory(): [NVIDIA] Reuse memory allocations instead of releasing and reallocating. A memory allocation has been released 0.011 seconds ago, and it could have been reused in place of this allocation.
        0x7f1922d7, // perf Both GPU Assisted Validation and Normal Core Check Validation are enabled, this is not recommend as it  will be very slow. Once all errors in Core Check are solved, please disable, then only use GPU-AV for best performance.
        0x24b5c69f, // Internal Warning: Ray Query validation option was enabled, but the rayQuery feature is not enabled. [Disabling gpuav_validate_ray_query]
        // 0x53c1342f, //OBS layer Validation Warning: [ BestPractices-Error-Result ] Object 0: handle = 0x1218c4eb6900, type = VK_OBJECT_TYPE_INSTANCE; | MessageID = 0x53c1342f | vkGetPhysicalDeviceImageFormatProperties2(): Returned error VK_ERROR_FORMAT_NOT_SUPPORTED.
        0x58a102d7,
        //	[0]  0x2cadaeb1900, type: 6, name : Frame Command Buffer(2)
        // Validation Performance Warning : [BestPractices - vkCmdEndRenderPass - redundant - attachment - on - tile] Object 0 : handle = 0x2cadaeb1900, name = Frame Command Buffer(2), type = VK_OBJECT_TYPE_COMMAND_BUFFER; | MessageID = 0x58a102d7 | vkCmdEndRenderPass() : [Arm] [IMG] : Render pass was ended, but attachment #0 (format: VK_FORMAT_R8G8B8A8_UNORM, untouched aspects VK_IMAGE_ASPECT_COLOR_BIT) was never accessed by a pipeline or clear command.On tile - based architectures, LOAD_OP_LOAD and STORE_OP_STORE consume bandwidth and should not be part of the render pass if the attachments are not intended to be accessed.
        // BestPractices - vkCmdEndRenderPass - redundant - attachment - on - tile(WARN / PERF) : msgNum: 1486947031 - Validation Performance Warning : [BestPractices - vkCmdEndRenderPass - redundant - attachment - on - tile] Object 0 : handle = 0x2cadaea6700, name = Frame Command Buffer(0), type = VK_OBJECT_TYPE_COMMAND_BUFFER; | MessageID = 0x58a102d7 | vkCmdEndRenderPass() : [Arm] [IMG] : Render pass was ended, but attachment #0 (format: VK_FORMAT_R8G8B8A8_UNORM, untouched aspects VK_IMAGE_ASPECT_COLOR_BIT) was never accessed by a pipeline or clear command.On tile - based architectures, LOAD_OP_LOAD and STORE_OP_STORE consume bandwidth and should not be part of the render pass if the attachments are not intended to be accessed.
        0x8adbf7e3, // BestPractices - Arm - vkCmdDrawIndexed - sparse - index - buffer(WARN / PERF) : msgNum: -1965295645 - Validation Performance Warning : [BestPractices - Arm - vkCmdDrawIndexed - sparse - index - buffer] | MessageID = 0x8adbf7e3 | vkCmdDrawIndexed() : [Arm] The indices which were specified for the draw call only utilise approximately 0.07 % of index buffer value range.Arm Mali architectures before G71 do not have IDVS(Index - Driven Vertex Shading), meaning all vertices corresponding to indices between the minimum and maximum would be loaded, and possibly shaded, whether or not they are used.

    };
    
    std::stringstream disabledMessages;

    void createMuteVUIDs()
    {
        disabledMessages << std::hex << std::showbase;
#ifdef _WIN32
        const char separator = ',';
#else
        const char separator = ':';
#endif
        for (int i = 0; i < muteVUIds.size(); ++i)
        {
            if (i != 0)
                disabledMessages << separator;

            disabledMessages << muteVUIds[i];
        }
    }

    std::string vuidsStr = "";
    const char *vuidsCStr = nullptr;

    VkBaseInStructure *buildPNextChain()
    {
        createMuteVUIDs();

        vuidsStr = disabledMessages.str();
        vuidsCStr = vuidsStr.c_str();

        layerSettings = std::vector<VkLayerSettingEXT>{
            {layerName, "fine_grained_locking", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "validate_core", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "validate_sync", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "thread_safety", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "check_image_layout", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "check_command_buffer", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "check_object_in_use", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "check_query", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "check_shaders", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "check_shaders_caching", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "unique_handles", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "object_lifetime", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "stateless_param", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "syncval_shader_accesses_heuristic", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "syncval_message_extra_properties", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "syncval_message_extra_properties_pretty_print", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "printf_enable", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &falsePtr},
            {layerName, "printf_verbose", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &falsePtr},
#if defined(_WIN32) && !defined(ERMY_XR_OPENXR)
            {layerName, "gpuav_enable", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "gpuav_image_layout", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "gpuav_shader_instrumentation", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
#else
            {layerName, "gpuav_enable", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &falsePtr},
            {layerName, "gpuav_image_layout", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &falsePtr},
            {layerName, "gpuav_shader_instrumentation", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &falsePtr},
#endif
            {layerName, "validate_best_practices", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "validate_best_practices_arm", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "validate_best_practices_amd", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "validate_best_practices_img", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "validate_best_practices_nvidia", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "enable_message_limit", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &truePtr},
            {layerName, "message_id_filter", VK_LAYER_SETTING_TYPE_STRING_EXT, 1, &vuidsCStr},
            {layerName, "duplicate_message_limit", VK_LAYER_SETTING_TYPE_UINT32_EXT, 1, &setting_duplicate_message_limit},
            {layerName, "debug_action", VK_LAYER_SETTING_TYPE_STRING_EXT, uint32_t(debug_action.size()), debug_action.data()},
            {layerName, "report_flags", VK_LAYER_SETTING_TYPE_STRING_EXT, uint32_t(report_flags.size()), report_flags.data()},
        };

        layerSettingsCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
            .settingCount = uint32_t(layerSettings.size()),
            .pSettings = layerSettings.data(),
        };

        return reinterpret_cast<VkBaseInStructure *>(&layerSettingsCreateInfo);
    }

    static constexpr const char *layerName{"VK_LAYER_KHRONOS_validation"};
    std::vector<VkLayerSettingEXT> layerSettings{};
    VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo{};
};

static ValidationSettings validationSettings = {};

extern mercury::ll::graphics::AdapterInfo GetInfoFromPhysicalDevice(VkPhysicalDevice physicalDevice);

void EnumerateAllPhysicalDevices()
{
    gAllPhysicalDevices = EnumerateVulkanObjects(gVKInstance, vkEnumeratePhysicalDevices); 
    
    MLOG_INFO(u8"Found %d physical devices", gAllPhysicalDevices.size());

    gAllAdaptersInfo.resize(gAllPhysicalDevices.size());

    for(int i = 0; i < gAllPhysicalDevices.size(); ++i)
    {
        gAllAdaptersInfo[i] = GetInfoFromPhysicalDevice(gAllPhysicalDevices[i]);
    } 
}

void Instance::Initialize()
{
    MLOG_DEBUG(u8"Initialize Graphics System (Vulkan)");

    auto &config = mercury::Application::GetCurrentApplication()->GetConfig();
   
    auto &graphicsCfg = config.graphics;
    gVKConfig = graphicsCfg.vkConfig;

    mercury::memory::ReservedAllocator::InitDesc initDesc;

    initDesc.bucketsInfo.push_back({16, 16_MB, 1_MB});
    initDesc.bucketsInfo.push_back({32, 16_MB, 1_MB});
    initDesc.bucketsInfo.push_back({64, 16_MB, 1_MB});
    initDesc.bucketsInfo.push_back({128, 16_MB, 1_MB});
    initDesc.bucketsInfo.push_back({256, 16_MB, 1_MB});
    initDesc.bucketsInfo.push_back({512, 16_MB, 1_MB});
    initDesc.bucketsInfo.push_back({1024, 16_MB, 1_MB});
    initDesc.bucketsInfo.push_back({2048, 16_MB, 1_MB});
    initDesc.bucketsInfo.push_back({4096, 16_MB, 1_MB});
    initDesc.bucketsInfo.push_back({8192, 16_MB, 1_MB});
    initDesc.bucketsInfo.push_back({16384, 16_MB, 1_MB});

    memory::gGraphicsMemoryAllocator = new mercury::memory::ReservedAllocator(initDesc);

    //gVKGlobalAllocationsCallbacks = new AllocationsCallbacks();
    LoadVK_Library();

    u32 installedVersion = 0;

    if (vkEnumerateInstanceVersion != nullptr)
    {
        VK_CALL(vkEnumerateInstanceVersion(&installedVersion));

        int major = VK_VERSION_MAJOR(installedVersion);
        int minor = VK_VERSION_MINOR(installedVersion);
        int patch = VK_VERSION_PATCH(installedVersion);

        MLOG_INFO(u8"Found vulkan instance: %d.%d.%d", major, minor, patch);
    }

    VKInstanceExtender instance_extender(installedVersion);

    bool isDebugLayers = false;
#ifndef MERCURY_RETAIL_BUILD
    isDebugLayers = graphicsCfg.enableValidationLayers;
#endif

    if (isDebugLayers)
    {
        instance_extender.TryAddLayer("VK_LAYER_KHRONOS_validation");
        instance_extender.TryAddExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    instance_extender.TryAddExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, Ver11);

#ifdef MERCURY_LL_OS_MACOS
    // Found drivers that contain devices which support the portability subset, but the instance does not enumerate portability drivers!
    // Applications that wish to enumerate portability drivers must set the VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR bit in the VkInstanceCreateInfo flags
    // and enable the VK_KHR_portability_enumeration instance extension.
    instance_extender.TryAddExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.flags = 0;
    debugCreateInfo.pNext = nullptr;
    debugCreateInfo.pUserData = nullptr;
    debugCreateInfo.pfnUserCallback = &debugCallback;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;

    VkApplicationInfo vkAppInfo;
    vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vkAppInfo.pNext = nullptr;
    vkAppInfo.pApplicationName = (const char*)config.appName;
    vkAppInfo.pEngineName = "mercury";
    vkAppInfo.applicationVersion = config.appVersion.packed;
    vkAppInfo.apiVersion = installedVersion ? installedVersion : VK_MAKE_API_VERSION(0, 1, 1, 0);

    vkSwapchainRequestInstanceExtensions(instance_extender);

    VkInstanceCreateInfo createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    if (isDebugLayers)
    {
        createInfo.pNext = &debugCreateInfo;
        debugCreateInfo.pNext = validationSettings.buildPNextChain();
    }

    createInfo.pApplicationInfo = &vkAppInfo;
    createInfo.ppEnabledLayerNames = instance_extender.EnabledLayers();
    createInfo.ppEnabledExtensionNames = instance_extender.EnabledExtensions();
    createInfo.enabledLayerCount = instance_extender.NumEnabledLayers();
    createInfo.enabledExtensionCount = instance_extender.NumEnabledExtension();

    createInfo.flags = 0;
#ifdef MERCURY_LL_OS_MACOS
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

   
    VK_CALL(vkCreateInstance(&createInfo, gVKGlobalAllocationsCallbacks, &gVKInstance));

    LoadVkInstanceLevelFuncs(gVKInstance);

    if (vkCreateDebugUtilsMessengerEXT != nullptr)
    {
        vkCreateDebugUtilsMessengerEXT(gVKInstance, &debugCreateInfo, gVKGlobalAllocationsCallbacks, &gVKDebugMessenger);
    }

    EnumerateAllPhysicalDevices();    
}

void Instance::Shutdown()
{
    MLOG_DEBUG(u8"Shutdown Graphics System (Vulkan)");
}

void *Instance::GetNativeHandle()
{
    return gVKInstance; // null instance has no native handle
}

#endif