#include "vk_utils.h"

#ifdef MERCURY_LL_GRAPHICS_VULKAN

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

#endif