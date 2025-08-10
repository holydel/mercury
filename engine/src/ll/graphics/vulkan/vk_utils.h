#pragma once

#include "mercury_vulkan.h"
#include "ll/graphics.h"

#ifdef MERCURY_LL_GRAPHICS_VULKAN

mercury::ll::graphics::AdapterInfo::Vendor GetVendorFromVkVendorID(mercury::u64 vendor_id);

#endif