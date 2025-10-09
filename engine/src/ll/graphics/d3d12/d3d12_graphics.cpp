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

// Include D3D12 Memory Allocator implementation in this translation unit
#include "../../../../engine/third_party/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp"

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
    static const char *backendName = "D3D12";
    return backendName;
}

#define RPC_NO_WINDOWS_H

// Helper functions for fence synchronization
static u64 Signal(ID3D12CommandQueue* commandQueue, ID3D12Fence* fence, u64& fenceValue)
{
    u64 fenceValueForSignal = ++fenceValue;
    commandQueue->Signal(fence, fenceValueForSignal);
    return fenceValueForSignal;
}

static void WaitForFenceValue(ID3D12Fence* fence, u64 fenceValue, HANDLE fenceEvent)
{
    if (fence->GetCompletedValue() < fenceValue)
    {
        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }
}

static void Flush(ID3D12CommandQueue* commandQueue, ID3D12Fence* fence, u64& fenceValue, HANDLE fenceEvent)
{
    u64 fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
    WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
}

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
    MLOG_DEBUG(u8"Shutdown Graphics System (D3D12)");
}

void *Instance::GetNativeHandle()
{
    return nullptr;
}

void Instance::AcquireAdapter(const AdapterSelectorInfo &selector_info)
{
    if (gAdapter == nullptr)
        gAdapter = new Adapter();
}

void *Adapter::GetNativeHandle()
{
    return nullptr;
}

void Adapter::CreateDevice()
{
    if (gDevice == nullptr)
        gDevice = new Device();
}

void Adapter::Initialize()
{
    MLOG_DEBUG(u8"Initialize Adapter (D3D12)");
}

void Adapter::Shutdown()
{
    MLOG_DEBUG(u8"Shutdown Adapter (D3D12)");
}

void Device::Initialize()
{
    MLOG_DEBUG(u8"Initialize Device (D3D12)");

    const auto &renderCfg = Application::GetCurrentApplication()->GetConfig().graphics;

    D3D_CALL(D3D12CreateDevice(gD3DAdapter, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&gD3DDevice)));

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
    allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED |
                          D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;

    HRESULT hr = D3D12MA::CreateAllocator(&allocatorDesc, &gAllocator);
}

void Device::Shutdown()
{
    MLOG_DEBUG(u8"Shutdown Device (D3D12)");
}

void Device::Tick()
{
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
    return gD3DDevice;
}

void *Swapchain::GetNativeHandle()
{
    return nullptr;
}

// Swapchain implementation with frame management
struct BackbufferResourceInfo
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE bbRTV;
    ID3D12Resource* bbResource = nullptr;
    int frameIndex = 0;
};

struct FrameData
{
    HANDLE fenceEvent = nullptr;
    ID3D12Fence* fence = nullptr;
    UINT64 fenceValue = 0;
    ID3D12GraphicsCommandList* commandList = nullptr;
    ID3D12CommandAllocator* commandAllocator = nullptr;
    u64 frameIndex = 0;
};

int gNumFrames = 3;
std::vector<BackbufferResourceInfo> gBBFrames;
std::vector<FrameData> gFrames;
IDXGISwapChain3* gSwapChain = nullptr;
u64 gCurrentBBResourceIndex = 0;
u32 gFrameRingCurrent = 0;
u64 gFrameID = 0;
u32 gNewWidth = 0;
u32 gCurWidth = 0;
u32 gNewHeight = 0;
u32 gCurHeight = 0;

static BackbufferResourceInfo& GetCurrentBackbufferResourceInfo()
{
    return gBBFrames[gCurrentBBResourceIndex];
}

void Swapchain::Initialize()
{
    HWND hwnd = static_cast<HWND>(os::gOS->GetCurrentNativeWindowHandle());

    RECT clientRect = {};
    GetClientRect(hwnd, (LPRECT)&clientRect);

    gNewWidth = gCurWidth = clientRect.right - clientRect.left;
    gNewHeight = gCurHeight = clientRect.bottom - clientRect.top;

    // Create swapchain
    DXGI_SWAP_CHAIN_DESC1 sdesc = {};
    sdesc.Width = gCurWidth;
    sdesc.Height = gCurHeight;
    sdesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    sdesc.BufferCount = gNumFrames;
    sdesc.Format = gD3DSwapChainFormat;
    sdesc.SampleDesc.Count = 1;
    sdesc.SampleDesc.Quality = 0;
    sdesc.Scaling = DXGI_SCALING_NONE;
    sdesc.Stereo = false;
    sdesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    IDXGISwapChain1* swapchain1 = nullptr;
    D3D_CALL(gD3DFactory->CreateSwapChainForHwnd(gD3DCommandQueue, hwnd, &sdesc, nullptr, nullptr, &swapchain1));
    D3D_CALL(swapchain1->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&gSwapChain));
    swapchain1->Release();

    // Create backbuffer resources
    gBBFrames.resize(gNumFrames);
    auto rtvDescriptorSize = gD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(gDescriptorsHeapRTV->GetCPUDescriptorHandleForHeapStart());

    for (uint32_t i = 0; i < gNumFrames; ++i)
    {
        BackbufferResourceInfo& bb = gBBFrames[i];
        gSwapChain->GetBuffer(i, IID_PPV_ARGS(&bb.bbResource));

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = gD3DSwapChainFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        gD3DDevice->CreateRenderTargetView(bb.bbResource, &rtvDesc, rtvHandle);
        bb.bbRTV = rtvHandle;
        bb.frameIndex = i;
        rtvHandle.Offset(rtvDescriptorSize);
    }

    // Initialize frame data for CPU-GPU synchronization
    gFrames.resize(gNumFrames);
    for (uint32_t i = 0; i < gNumFrames; ++i)
    {
        FrameData& frame = gFrames[i];
        D3D_CALL(gD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frame.fence)));
        D3D_CALL(gD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.commandAllocator)));
        D3D_CALL(gD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frame.commandAllocator, nullptr, IID_PPV_ARGS(&frame.commandList)));
        D3D_CALL(frame.commandList->Close());
        
        frame.fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        frame.fenceValue = 0;
        frame.frameIndex = i;
    }
}

void Swapchain::Shutdown()
{
    // Wait for all frames to complete
    for (auto& frame : gFrames)
    {
        if (frame.fence)
        {
            Flush(gD3DCommandQueue, frame.fence, frame.fenceValue, frame.fenceEvent);
            frame.fence->Release();
            ::CloseHandle(frame.fenceEvent);
        }
        if (frame.commandList) frame.commandList->Release();
        if (frame.commandAllocator) frame.commandAllocator->Release();
    }
    gFrames.clear();

    for (auto& bb : gBBFrames)
    {
        if (bb.bbResource) bb.bbResource->Release();
    }
    gBBFrames.clear();

    if (gSwapChain)
    {
        gSwapChain->Release();
        gSwapChain = nullptr;
    }
}

CommandList Swapchain::AcquireNextImage()
{
    // Check if resize is needed
    RECT clientRect = {};
    HWND hwnd = static_cast<HWND>(os::gOS->GetCurrentNativeWindowHandle());
    GetClientRect(hwnd, (LPRECT)&clientRect);
    
    gNewWidth = clientRect.right - clientRect.left;
    gNewHeight = clientRect.bottom - clientRect.top;

    if (gNewWidth != gCurWidth || gCurHeight != gNewHeight && gNewWidth > 0 && gNewHeight > 0)
    {
        // Flush all frames before resize
        for (auto& frame : gFrames)
        {
            Flush(gD3DCommandQueue, frame.fence, frame.fenceValue, frame.fenceEvent);
        }

        gCurWidth = gNewWidth;
        gCurHeight = gNewHeight;

        // Release old backbuffer resources
        for (auto& bb : gBBFrames)
        {
            if (bb.bbResource)
            {
                bb.bbResource->Release();
                bb.bbResource = nullptr;
            }
        }

        // Resize swapchain
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        D3D_CALL(gSwapChain->GetDesc(&swapChainDesc));
        D3D_CALL(gSwapChain->ResizeBuffers(swapChainDesc.BufferCount, gCurWidth, gCurHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

        // Recreate backbuffer RTVs
        auto rtvDescriptorSize = gD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(gDescriptorsHeapRTV->GetCPUDescriptorHandleForHeapStart());

        for (uint32_t i = 0; i < gNumFrames; ++i)
        {
            BackbufferResourceInfo& bb = gBBFrames[i];
            gSwapChain->GetBuffer(i, IID_PPV_ARGS(&bb.bbResource));

            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = gD3DSwapChainFormat;
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            gD3DDevice->CreateRenderTargetView(bb.bbResource, &rtvDesc, rtvHandle);
            bb.bbRTV = rtvHandle;
            rtvHandle.Offset(rtvDescriptorSize);
        }
    }

    auto& frame = gFrames[gFrameRingCurrent];

    // Wait for this frame's fence
    WaitForFenceValue(frame.fence, frame.fenceValue, frame.fenceEvent);

    // Reset command allocator and list
    frame.commandAllocator->Reset();
    frame.commandList->Reset(frame.commandAllocator, nullptr);

    // Get current backbuffer index
    gCurrentBBResourceIndex = gSwapChain->GetCurrentBackBufferIndex();
    gFrameID++;

    auto& bbResource = GetCurrentBackbufferResourceInfo();

    // Transition to render target
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        bbResource.bbResource,
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    frame.commandList->ResourceBarrier(1, &barrier);

    // Clear and set render target
    auto clearColor = mercury::ll::graphics::gSwapchain->clearColor;

    FLOAT clearColorD3D[] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
    frame.commandList->ClearRenderTargetView(bbResource.bbRTV, clearColorD3D, 0, nullptr);
    frame.commandList->OMSetRenderTargets(1, &bbResource.bbRTV, FALSE, nullptr);

    // Set viewport and scissor
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)gCurWidth, (float)gCurHeight, 0.0f, 1.0f };
    D3D12_RECT scissorRect = { 0, 0, (LONG)gCurWidth, (LONG)gCurHeight };
    frame.commandList->RSSetViewports(1, &viewport);
    frame.commandList->RSSetScissorRects(1, &scissorRect);

    CommandList result;
    result.nativePtr = frame.commandList;
    return result;
}

void Swapchain::Present()
{
    auto& frame = gFrames[gFrameRingCurrent];
    auto& bbResource = GetCurrentBackbufferResourceInfo();

    // Transition back to present
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        bbResource.bbResource,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    frame.commandList->ResourceBarrier(1, &barrier);

    // Close and execute command list
    D3D_CALL(frame.commandList->Close());

    ID3D12CommandList* const commandLists[] = { frame.commandList };
    gD3DCommandQueue->ExecuteCommandLists(1, commandLists);

    // Present
    gSwapChain->Present(1, 0);

    // Signal fence
    frame.fenceValue = Signal(gD3DCommandQueue, frame.fence, frame.fenceValue);

    // Move to next frame
    gFrameRingCurrent = (gFrameRingCurrent + 1) % gNumFrames;
}

void Swapchain::SetFullscreen(bool fullscreen)
{
    if (gSwapChain)
    {
        gSwapChain->SetFullscreenState(fullscreen, nullptr);
    }
}

int Swapchain::GetWidth() const
{
    return gCurWidth;
}

int Swapchain::GetHeight() const
{
    return gCurHeight;
}

u8 Swapchain::GetNumberOfFrames()
{
    return static_cast<u8>(gNumFrames);
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
    // In D3D12, command lists are managed per-frame, not via pools
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
    auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);
    cmdListD3D12->SetPipelineState(gAllPSOs[psoID.handle]);
    cmdListD3D12->SetGraphicsRootSignature(gAllSignatures[psoID.handle]);
	cmdListD3D12->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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
    ShaderHandle result;
    result.handle = static_cast<u32>(gAllShaders.size());
    gAllShaders.emplace_back(bytecode.data, bytecode.size);
    return result;
}

void Device::UpdateShaderModule(ShaderHandle shaderModuleID, const ShaderBytecodeView& bytecode)
{
    gAllShaders[shaderModuleID.handle] = CD3DX12_SHADER_BYTECODE(bytecode.data, bytecode.size);
}

void Device::DestroyShaderModule(ShaderHandle shaderModuleID)
{
    // In D3D12, shader bytecode is just data, no explicit cleanup needed
}

PsoHandle Device::CreateRasterizePipeline(const mercury::ll::graphics::RasterizePipelineDescriptor& desc)
{
    // TODO: Implement full D3D12 rasterize pipeline creation
    PsoHandle result;
    result.handle = static_cast<u32>(gAllPSOs.size());

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, rootSignatureFlags);

    ID3DBlob* signature = nullptr;
    ID3DBlob* error = nullptr;
    D3D_CALL(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));

    gAllSignatures.emplace_back(nullptr);
    gAllPSOs.emplace_back(nullptr);

    D3D_CALL(gD3DDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&gAllSignatures[result.handle])));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    
    psoDesc.pRootSignature = gAllSignatures[result.handle];
    
    if (desc.vertexShader.isValid())
    {
        psoDesc.VS = gAllShaders[desc.vertexShader.handle];
    }

    if (desc.tessControlShader.isValid())
    {
		psoDesc.HS = gAllShaders[desc.tessControlShader.handle];
    }

    if (desc.tessEvalShader.isValid())
    {
		psoDesc.DS = gAllShaders[desc.tessEvalShader.handle];
    }

    if (desc.geometryShader.isValid())
    {
        psoDesc.GS = gAllShaders[desc.geometryShader.handle];
	}

    if (desc.fragmentShader.isValid())
    {
        psoDesc.PS = gAllShaders[desc.fragmentShader.handle];
	}


    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FrontCounterClockwise = true;

    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthEnable = false;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    //psoDesc.DepthStencilState.DepthEnable = FALSE;
    //psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    //psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    D3D_CALL(gD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&gAllPSOs[result.handle])));

    return result;
}

void Device::UpdatePipelineState(PsoHandle psoID, const mercury::ll::graphics::RasterizePipelineDescriptor& desc)
{
    // TODO: Implement D3D12 pipeline state update
}

void Device::DestroyRasterizePipeline(PsoHandle psoID)
{
    gAllPSOs[psoID.handle]->Release();
    gAllPSOs[psoID.handle] = nullptr;

    gAllSignatures[psoID.handle]->Release();
    gAllSignatures[psoID.handle] = nullptr;
}

CommandPool Device::CreateCommandPool(QueueType queue_type)
{
    CommandPool pool;
    // In D3D12, command pools are managed per-frame, not explicitly created
    return pool;
}

TimelineSemaphore Device::CreateTimelineSemaphore(mercury::u64 initial_value)
{
    TimelineSemaphore semaphore;
    // D3D12 uses fences instead of timeline semaphores
    ID3D12Fence* fence = nullptr;
    D3D_CALL(gD3DDevice->CreateFence(initial_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    semaphore.nativePtr = fence;
    return semaphore;
}

void Device::WaitIdle()
{
    for (auto& frame : gFrames)
    {
        Flush(gD3DCommandQueue, frame.fence, frame.fenceValue, frame.fenceEvent);
    }
}

void Device::WaitQueueIdle(QueueType queue_type)
{
    WaitIdle();
}

void Device::SetDebugName(const char* utf8_name)
{
    if (gD3DDevice)
    {
        wchar_t wideName[256] = {};
        MultiByteToWideChar(CP_UTF8, 0, utf8_name, -1, wideName, 256);
        gD3DDevice->SetName(wideName);
    }
}

#endif