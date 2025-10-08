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

// Swapchain dimensions (temporary placeholder until proper swapchain implementation)
int gInitialCanvasWidth = 1024;
int gInitialCanvasHeight = 768;

Device *gDevice = nullptr;
Instance *gInstance = nullptr;
Adapter *gAdapter = nullptr;
Swapchain *gSwapchain = nullptr;

void CommandList::RenderImgui()
{
    auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);

    cmdListD3D12->SetDescriptorHeaps(1, &gImgui_pd3dSrvDescHeap);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdListD3D12);
}

void CommandList::SetPSO(Handle<u32> psoID)
{
    // TODO: Implement D3D12 pipeline state binding
    // auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);
    // cmdListD3D12->SetPipelineState(gAllPSOs[psoID.handle].pipelineState);
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
    // TODO: Implement D3D12 shader module creation
    ShaderHandle result;
    result.handle = 0; // Placeholder
    return result;
}

void Device::UpdateShaderModule(ShaderHandle shaderModuleID, const ShaderBytecodeView& bytecode)
{
    // TODO: Implement D3D12 shader module update
}

void Device::DestroyShaderModule(ShaderHandle shaderModuleID)
{
    // TODO: Implement D3D12 shader module destruction
}

PsoHandle Device::CreateRasterizePipeline(const RasterizePipelineDescriptor& desc)
{
    // TODO: Implement D3D12 rasterize pipeline creation
    PsoHandle result;
    result.handle = 0; // Placeholder
    return result;
}

void Device::UpdatePipelineState(PsoHandle psoID, const RasterizePipelineDescriptor& desc)
{
    // TODO: Implement D3D12 pipeline state update
}

void Device::DestroyRasterizePipeline(PsoHandle psoID)
{
    // TODO: Implement D3D12 rasterize pipeline destruction
}

int Swapchain::GetWidth() const
{
    // TODO: Get actual swapchain width from D3D12 swapchain
    return gInitialCanvasWidth; // Placeholder - use actual swapchain desc
}

int Swapchain::GetHeight() const
{
    // TODO: Get actual swapchain height from D3D12 swapchain
    return gInitialCanvasHeight; // Placeholder - use actual swapchain desc
}

#endif
