#include "d3d12_graphics.h"

#if defined(MERCURY_LL_GRAPHICS_D3D12)

#include "mercury_memory.h"
#include "mercury_log.h"
#include "mercury_application.h"
#include "mercury_utils.h"

using namespace mercury;
using namespace mercury::ll::graphics;

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

IDXGIFactory4 *gD3DFactory = nullptr;
ID3D12Debug1 *gDebugController = nullptr;
IDXGIAdapter1 *gD3DAdapter = nullptr;
ID3D12Device *gD3DDevice = nullptr;
ID3D12DebugDevice *gD3DDebugDevice = nullptr;
ID3D12CommandQueue *gD3DCommandQueue = nullptr;
ID3D12CommandAllocator *gD3DCommandAllocator = nullptr;
ID3D12DescriptorHeap *gDescriptorsHeapRTV = nullptr;
ID3D12DescriptorHeap *gDescriptorsHeapDSV = nullptr;

D3D12MA::Allocator *gAllocator = nullptr;
DXGI_FORMAT gD3DSwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

Device *gDevice = nullptr;
Instance *gInstance = nullptr;
Adapter *gAdapter = nullptr;
Swapchain *gSwapchain = nullptr;

mercury::memory::ReservedAllocator *memory::gGraphicsMemoryAllocator = nullptr;

const char *ll::graphics::GetBackendName()
{
    static const char *backendName = "NULL";
    return backendName;
}

#define RPC_NO_WINDOWS_H

void Instance::Initialize()
{
    MLOG_DEBUG(u8"Initialize Graphics System (D3D12)");

    const auto &renderCfg = Application::GetCurrentApplication()->GetConfig().graphics;

    UINT dxgiFactoryFlags = 0;

    if (renderCfg.enableValidationLayers)
    {
        ID3D12Debug *dc;
        D3D_CALL(D3D12GetDebugInterface(IID_PPV_ARGS(&dc)));
        D3D_CALL(dc->QueryInterface(IID_PPV_ARGS(&gDebugController)));
        gDebugController->EnableDebugLayer();
        gDebugController->SetEnableGPUBasedValidation(true);

        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

        dc->Release();
        dc = nullptr;
    }

    D3D_CALL(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&gD3DFactory)));

    IDXGIAdapter1* adapter = nullptr;
    std::vector<DXGI_ADAPTER_DESC1> allAdapterDescs;
    allAdapterDescs.reserve(8);

    for (UINT i = 0; DXGI_ERROR_NOT_FOUND != gD3DFactory->EnumAdapters1(i, &adapter); ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        allAdapterDescs.push_back(desc);

        c8 descStrUtf8[256] = {};
        mercury::utils::string::utf16_to_utf8((const c16*)desc.Description, descStrUtf8, 256);
        MLOG_DEBUG(u8"Found D3D12 device (%d): %s", i, descStrUtf8);
        adapter->Release();
    }

    auto selectedAdapterID =  renderCfg.explicitAdapterIndex == 255 ? 0 : renderCfg.explicitAdapterIndex;

    for (UINT i = 0; DXGI_ERROR_NOT_FOUND != gD3DFactory->EnumAdapters1(i, &adapter); ++i)
    {
        if (i == selectedAdapterID)
        {
            gD3DAdapter = adapter;
            auto& desc = allAdapterDescs[i];

            c8 descStrUtf8[256] = {};
            mercury::utils::string::utf16_to_utf8((const c16*)desc.Description, descStrUtf8, 256);
            MLOG_DEBUG(u8"Selected D3D12 device (%d): %s", i, descStrUtf8);
            break;
        }
        else
        {
            adapter->Release();
        }
    }
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

    const auto &renderCfg = Application::GetCurrentApplication()->GetConfig().graphics;

    D3D_CALL(D3D12CreateDevice(gD3DAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&gD3DDevice)));

    if (renderCfg.enableValidationLayers)
    {
        D3D_CALL(gD3DDevice->QueryInterface(&gD3DDebugDevice));
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    D3D_CALL(gD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&gD3DCommandQueue)));

    D3D_CALL(gD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gD3DCommandAllocator)));

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = 64;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        D3D_CALL(gD3DDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gDescriptorsHeapRTV)));
        gDescriptorsHeapRTV->SetName(L"RTV Heap");
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = 64;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        D3D_CALL(gD3DDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gDescriptorsHeapDSV)));
        gDescriptorsHeapDSV->SetName(L"DSV Heap");
    }

    D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
    allocatorDesc.pDevice = gD3DDevice;
    allocatorDesc.pAdapter = gD3DAdapter;
    // These flags are optional but recommended.
    allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED |
                          D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;

    HRESULT hr = D3D12MA::CreateAllocator(&allocatorDesc, &gAllocator);
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

void *Device::GetNativeHandle()
{
    return nullptr; // null device has no native handle
}

void *Swapchain::GetNativeHandle()
{
    return nullptr;
}

void Swapchain::Initialize(void *native_window_handle)
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