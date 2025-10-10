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

void* Device::GetNativeHandle()
{
    return nullptr; // null device has no native handle
}

void *Swapchain::GetNativeHandle()
{
    return nullptr;
}

void Swapchain::Initialize()
{
    // null implementation - do nothing
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

int Swapchain::GetWidth() const
{
    return 800; // default width for null implementation
}

int Swapchain::GetHeight() const
{
    return 600; // default height for null implementation
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

// Missing functions for testbed compatibility
ShaderHandle Device::CreateShaderModule(const ShaderBytecodeView& bytecode)
{
    ShaderHandle result;
    result.handle = 0; // null implementation
    return result;
}

void Device::UpdateShaderModule(ShaderHandle shaderModuleID, const ShaderBytecodeView& bytecode)
{
    // null implementation - do nothing
}

void Device::DestroyShaderModule(ShaderHandle shaderModuleID)
{
    // null implementation - do nothing
}

PsoHandle Device::CreateRasterizePipeline(const RasterizePipelineDescriptor& desc)
{
    PsoHandle result;
    result.handle = 0; // null implementation
    return result;
}

void Device::UpdatePipelineState(PsoHandle psoID, const RasterizePipelineDescriptor& desc)
{
    // null implementation - do nothing
}

void Device::DestroyRasterizePipeline(PsoHandle psoID)
{
    // null implementation - do nothing
}

void CommandList::SetPSO(Handle<u32> psoID)
{
    // null implementation - do nothing
}

void CommandList::Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
{
    // null implementation - do nothing
}

void CommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
    // null implementation - do nothing
}

void CommandList::SetScissor(i32 x, i32 y, u32 width, u32 height)
{
    // null implementation - do nothing
}

void CommandList::RenderImgui()
{
    // null implementation - do nothing
}

void Device::ImguiInitialize()
{
    // null implementation - do nothing
}

void Device::ImguiRegenerateFontAtlas()
{
    // null implementation - do nothing
}

void Device::ImguiNewFrame()
{
    // null implementation - do nothing
}

void Device::ImguiShutdown()
{
    // null implementation - do nothing
}

// Embedded shaders for testbed compatibility
namespace mercury::ll::graphics::embedded_shaders {

mercury::ll::graphics::ShaderBytecodeView TestTriangleVS()
{
    static const char data[] = "// null shader";
    return { data, sizeof(data) };
}

mercury::ll::graphics::ShaderBytecodeView TestTrianglePS()
{
    static const char data[] = "// null shader";
    return { data, sizeof(data) };
}

} // namespace mercury::ll::graphics::embedded_shaders

#endif