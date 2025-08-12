#include "vk_utils.h"

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include "vk_graphics.h"

mercury::ll::graphics::AdapterInfo::Vendor GetVendorFromVkVendorID(mercury::u64 vendor_id)
{
    
    switch (vendor_id)
    {
    case 0x10DE: return mercury::ll::graphics::AdapterInfo::Vendor::NVIDIA;
    case 0x1002: return mercury::ll::graphics::AdapterInfo::Vendor::AMD;
    case 0x8086: return mercury::ll::graphics::AdapterInfo::Vendor::Intel;
    case 0x13B5: return mercury::ll::graphics::AdapterInfo::Vendor::Qualcomm;
    case 0x1AE0: return mercury::ll::graphics::AdapterInfo::Vendor::ARM;
    default: return mercury::ll::graphics::AdapterInfo::Vendor::Unknown;
    }
}

void vk_utils::debug::_setObjectName(mercury::u64 objHandle, VkObjectType objType, const char* name)
{
	if (vkSetDebugUtilsObjectNameEXT)
	{
		VkDebugUtilsObjectNameInfoEXT info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		info.objectHandle = objHandle;
		info.objectType = objType;
		info.pObjectName = name;

		VK_CALL(vkSetDebugUtilsObjectNameEXT(gVKDevice, &info));
	}
}

#endif