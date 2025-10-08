#include "d3d12_graphics.h"

#if defined(MERCURY_LL_GRAPHICS_D3D12)

#include "mercury_memory.h"
#include "mercury_log.h"
#include "mercury_application.h"
#include "mercury_utils.h"

#include "../../../imgui/imgui_impl.h"

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
ID3D12DescriptorHeap* gImgui_pd3dSrvDescHeap = nullptr;

D3D12MA::Allocator *gAllocator = nullptr;
DXGI_FORMAT gD3DSwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

// Global storage for shaders, signatures, and PSOs
std::vector<CD3DX12_SHADER_BYTECODE> gAllShaders;
std::vector<ID3D12RootSignature*> gAllSignatures;
std::vector<ID3D12PipelineState*> gAllPSOs;

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

void Device::ImguiInitialize()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 20;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    gD3DDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gImgui_pd3dSrvDescHeap));

    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.CommandQueue = gD3DCommandQueue;
    init_info.Device = gD3DDevice;
    init_info.SrvDescriptorHeap = gImgui_pd3dSrvDescHeap;
    init_info.NumFramesInFlight = 3;
    init_info.RTVFormat = gD3DSwapChainFormat;
    init_info.LegacySingleSrvCpuDescriptor = gImgui_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
    init_info.LegacySingleSrvGpuDescriptor = gImgui_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart();
    ImGui_ImplDX12_Init(&init_info);

    ImGui_ImplDX12_CreateDeviceObjects();
}

void Device::ImguiShutdown()
{
    ImGui_ImplDX12_Shutdown();
}

void Device::ImguiNewFrame()
{
    ImGui_ImplDX12_NewFrame();
}

void Device::ImguiRegenerateFontAtlas()
{
    ImGui_ImplDX12_CreateDeviceObjects();
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

struct BackbufferResourceInfo
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE bbRTV = 0;
    ID3D12Resource* bbResource = 0;
    int frameIndex = 0;
};

int gNumFrames = 2;
std::vector<BackbufferResourceInfo> gBBFrames;
IDXGISwapChain3* gSwapChain = nullptr;
u64 gCurrentBBResourceIndex = 0;
u64 gFrameID = 0;
DXGI_FORMAT gSwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
u32 gNewWidth = 0;
u32 gCurWidth = 0;
u32 gNewHeight = 0;
u32 gCurHeight = 0;
u32 gDeltaFrameInBB = 0;

void Swapchain::Initialize()
{
    HWND hwnd = static_cast<HWND>(os::gOS->GetCurrentNativeWindowHandle());

    RECT clientRect = {};
    GetClientRect(hwnd, (LPRECT)&clientRect);

    int gNewWidth = clientRect.right - clientRect.left;
    int gNewHeight = clientRect.bottom - clientRect.top;

    DXGI_SWAP_CHAIN_DESC1 sdesc = {};
    sdesc.Width = gNewWidth = gCurWidth = clientRect.right - clientRect.left;
    sdesc.Height = gNewHeight = gCurHeight = clientRect.bottom - clientRect.top;
    sdesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    sdesc.BufferCount = gNumFrames;
    sdesc.Format = gSwapChainFormat;
    sdesc.SampleDesc.Count = 1;
    sdesc.SampleDesc.Quality = 0;
    sdesc.Scaling = DXGI_SCALING_NONE;
    sdesc.Stereo = false;
    sdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC sfdesc = {};
    sfdesc.Windowed = true;
    sfdesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    sfdesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sfdesc.RefreshRate.Numerator = 1;
    sfdesc.RefreshRate.Denominator = 60;
    IDXGIOutput* output = nullptr;

    IDXGISwapChain1* swapchain1 = nullptr;
    auto res = gD3DFactory->CreateSwapChainForHwnd(gD3DCommandQueue, hwnd, &sdesc, nullptr, output, &swapchain1);
    swapchain1->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&gSwapChain);

    gBBFrames.resize(gNumFrames);

    auto rtvDescriptorSize = gD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(gDescriptorsHeapRTV->GetCPUDescriptorHandleForHeapStart());

    for (uint32_t i = 0; i < gNumFrames; ++i)
    {
        BackbufferResourceInfo& bb = gBBFrames[i];

        gSwapChain->GetBuffer(i, IID_PPV_ARGS(&bb.bbResource));

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS; // Multisampled view

        gD3DDevice->CreateRenderTargetView(bb.bbResource, &rtvDesc, rtvHandle);

        bb.bbRTV = rtvHandle;
        rtvHandle.Offset(rtvDescriptorSize);
    }
}

void Swapchain::Shutdown()
{
}

CommandList Swapchain::AcquireNextImage()
{
    gCurrentBBResourceIndex = gSwapChain->GetCurrentBackBufferIndex();
	gFrameID++;
    return CommandList();
}

void Swapchain::Present()
{
    gSwapChain->Present(1, DXGI_SWAP_EFFECT_DISCARD);
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

void CommandList::RenderImgui()
{
    auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);

    cmdListD3D12->SetDescriptorHeaps(1, &gImgui_pd3dSrvDescHeap);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdListD3D12);
}

void CommandList::SetPSO(Handle<u32> psoID)
{
    // TODO: Implement D3D12 pipeline state binding
    auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);
    if (psoID.handle < gAllPSOs.size() && gAllPSOs[psoID.handle] != nullptr)
    {
        cmdListD3D12->SetPipelineState(gAllPSOs[psoID.handle]);
    }
}

void CommandList::Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
{
    auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);
    cmdListD3D12->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
    auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);
    
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = minDepth;
    viewport.MaxDepth = maxDepth;
    
    cmdListD3D12->RSSetViewports(1, &viewport);
}

void CommandList::SetScissor(i32 x, i32 y, u32 width, u32 height)
{
    auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);
    
    D3D12_RECT scissorRect = {};
    scissorRect.left = x;
    scissorRect.top = y;
    scissorRect.right = x + width;
    scissorRect.bottom = y + height;
    
    cmdListD3D12->RSSetScissorRects(1, &scissorRect);
}

ShaderHandle Device::CreateShaderModule(const ShaderBytecodeView& bytecode)
{
    // TODO: Implement D3D12 shader module creation - storing bytecode for later use
    ShaderHandle result;
    result.handle = static_cast<u32>(gAllShaders.size());
    gAllShaders.emplace_back(bytecode.data, bytecode.size);
    return result;
}

void Device::UpdateShaderModule(ShaderHandle shaderModuleID, const ShaderBytecodeView& bytecode)
{
    // TODO: Implement D3D12 shader module update
    if (shaderModuleID.handle < gAllShaders.size())
    {
        gAllShaders[shaderModuleID.handle] = CD3DX12_SHADER_BYTECODE(bytecode.data, bytecode.size);
    }
}

void Device::DestroyShaderModule(ShaderHandle shaderModuleID)
{
    // TODO: Implement D3D12 shader module destruction
    // In D3D12, shader bytecode is just data, no explicit cleanup needed
}

PsoHandle Device::CreateRasterizePipeline(const mercury::ll::graphics::RasterizePipelineDescriptor& desc)
{
    // TODO: Implement D3D12 rasterize pipeline creation
    PsoHandle result;
    result.handle = static_cast<u32>(gAllPSOs.size());
    gAllPSOs.push_back(nullptr); // Placeholder
    return result;
}

void Device::UpdatePipelineState(PsoHandle psoID, const mercury::ll::graphics::RasterizePipelineDescriptor& desc)
{
    // TODO: Implement D3D12 pipeline state update
}

void Device::DestroyRasterizePipeline(PsoHandle psoID)
{
    // TODO: Implement D3D12 rasterize pipeline destruction
    if (psoID.handle < gAllPSOs.size() && gAllPSOs[psoID.handle] != nullptr)
    {
        gAllPSOs[psoID.handle]->Release();
        gAllPSOs[psoID.handle] = nullptr;
    }
}

int Swapchain::GetWidth() const
{
    // TODO: Get actual swapchain width from D3D12 swapchain
    return 1024; // Placeholder - use actual swapchain desc
}

int Swapchain::GetHeight() const
{
    // TODO: Get actual swapchain height from D3D12 swapchain
    return 768; // Placeholder - use actual swapchain desc
}

#endif