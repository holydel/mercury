#include "ll/graphics.h"
#include "mercury_memory.h"

using namespace mercury;
using namespace mercury::ll::graphics;

#if defined(MERCURY_LL_GRAPHICS_NULL)
#include "mercury_log.h"

Device *gDevice = nullptr;
Instance *gInstance = nullptr;
Adapter *gAdapter = nullptr;
Swapchain *gSwapchain = nullptr;

mercury::memory::ReservedAllocator* memory::gGraphicsMemoryAllocator = nullptr;

const char* ll::graphics::GetBackendName()
{
    static const char* backendName = "NULL";
    return backendName;
}

void Instance::Initialize()
{
    MLOG_DEBUG(u8"Initialize Graphics System (NULL)");
}

void Instance::Shutdown()
{
    MLOG_DEBUG(u8"Shutdown Graphics System (NULL)");
}

void *Instance::GetNativeHandle()
{
    return nullptr; // null instance has no native handle
}

void Instance::AcquireAdapter(const AdapterSelectorInfo &selector_info)
{
    if (gAdapter == nullptr)
        gAdapter = new Adapter();
}

void *Adapter::GetNativeHandle()
{
    return nullptr; // null adapter has no native handle
}

void Adapter::CreateDevice()
{
    if (gDevice == nullptr)
        gDevice = new Device();
}

void Adapter::Initialize()
{
    MLOG_DEBUG(u8"Initialize Adapter (NULL)");
}

void Adapter::Shutdown()
{
    MLOG_DEBUG(u8"Shutdown Adapter (NULL)");
}

void Device::Initialize()
{
    MLOG_DEBUG(u8"Initialize Device (NULL)");
}

void Device::Shutdown()
{
    MLOG_DEBUG(u8"Shutdown Device (NULL)");
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

void* Device::GetNativeHandle()
{
    return nullptr; // null device has no native handle
}

void *Swapchain::GetNativeHandle()
{
    return nullptr;
}

void Swapchain::Initialize(void* native_window_handle)
{
}

void Swapchain::Shutdown()
{
}

CommandList Swapchain::AcquireNextImage()
{
    return CommandList();
}

void Swapchain::Present()
{
}

void Swapchain::SetFullscreen(bool fullscreen)
{
}

void TimelineSemaphore::WaitUntil(mercury::u64 value, mercury::u64 timeout)
{
}

void TimelineSemaphore::SetDebugName(const char *utf8_name)
{
}

void TimelineSemaphore::Destroy()
{
     nativePtr = nullptr;
}

void RenderPass::SetDebugName(const char *utf8_name)
{
}

void RenderPass::Destroy()
{
    nativePtr = nullptr;
}

bool CommandList::IsExecuted()
{
    return false;
}

void CommandList::SetDebugName(const char *utf8_name)
{
}

void CommandList::Destroy()
{
    nativePtr = nullptr;
}

CommandList CommandPool::AllocateCommandList()
{
    CommandList result;
    return result;
}

void CommandPool::SetDebugName(const char *utf8_name)
{    
}

void CommandPool::Destroy()
{    
    nativePtr = nullptr;
}

void CommandPool::Reset()
{    
}

#endif