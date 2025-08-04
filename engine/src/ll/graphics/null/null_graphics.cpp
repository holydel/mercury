#include "ll/graphics.h"
using namespace mercury;
using namespace mercury::ll::graphics;

#if defined(MERCURY_LL_GRAPHICS_NULL)
#include "mercury_log.h"

Device *gDevice = nullptr;
Instance *gInstance = nullptr;
Adapter *gAdapter = nullptr;
Swapchain *gSwapchain = nullptr;

void Instance::Initialize() {
    MLOG_DEBUG(u8"Initialize Graphics System (NULL)");
}

void Instance::Shutdown() {
    MLOG_DEBUG(u8"Shutdown Graphics System (NULL)");
}

u8 Instance::GetAdapterCount() {
    return 1; //fake null adapter
}

Adapter *Instance::AcquireAdapter(const AdapterSelectorInfo &selector_info) {

    if(gAdapter == nullptr)
        gAdapter = new Adapter();

    return gAdapter;
}

Device *Adapter::CreateDevice() {
    if(gDevice == nullptr)
        gDevice = new Device();

    return gDevice;
}

void Adapter::Initialize() {
    MLOG_DEBUG(u8"Initialize Adapter (NULL)");
}

void Adapter::Shutdown() {
    MLOG_DEBUG(u8"Shutdown Adapter (NULL)");
}

void Device::Initialize() {
    MLOG_DEBUG(u8"Initialize Device (NULL)");
}

void Device::Shutdown() {
    MLOG_DEBUG(u8"Shutdown Device (NULL)");
}

void Device::Tick() {
    //MLOG_DEBUG(u8"Tick Device (NULL)");
}
#endif