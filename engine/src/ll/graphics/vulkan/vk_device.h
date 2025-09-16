#include "ll/graphics.h"

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include "mercury_vulkan.h"
#include "mercury_application.h"

struct DeviceEnabledExtensions
{
	bool NvDecompressMemory : 1 = false;
	bool KhrBufferDeviceAddress : 1 = false;
	bool KhrDedicatedAllocation : 1 = false;
	bool KhrSynchronization2 : 1 = false;
	bool KhrTimelineSemaphore : 1 = false;
	bool ExtMemoryPriority : 1 = false;
	bool ExtPageableDeviceLocalMemory : 1 = false;
	bool KhrDynamicRendering : 1 = false;
	bool KhrDynamicRenderingLocalRead : 1 = false;
	bool KhrFragmentShaderBarycentric : 1 = false;
	bool NvFragmentShaderBarycentric : 1 = false;
};

extern DeviceEnabledExtensions gVKDeviceEnabledExtensions;

#endif