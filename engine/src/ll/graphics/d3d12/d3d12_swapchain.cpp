#include "d3d12_swapchain.h"

#ifdef MERCURY_LL_GRAPHICS_D3D12

#include "mercury_log.h"
#include "d3d12_utils.h"

using namespace mercury;
using namespace mercury::ll::graphics;

// Swapchain implementation with frame management
struct BackbufferResourceInfo
{
	int frameIndex = 0;
};


int gNumFrames = 3;
std::vector<BackbufferResourceInfo> gBBFrames;


IDXGISwapChain3* gSwapChain = nullptr;
u64 gCurrentBBResourceIndex = 0;
u64 gFrameID = 0;
u32 gNewWidth = 0;
u32 gCurWidth = 0;
u32 gNewHeight = 0;
u32 gCurHeight = 0;


RenderTargetHandle gSwapchainRenderTargetHandle;

static BackbufferResourceInfo& GetCurrentBackbufferResourceInfo()
{
	return gBBFrames[gCurrentBBResourceIndex];
}


void* Swapchain::GetNativeHandle()
{
	return nullptr;
}

void Swapchain::Initialize()
{
	void* windowHandle = os::gOS->GetCurrentNativeWindowHandle();

	ll::os::gOS->GetActualWindowSize((unsigned int&)gNewWidth, (unsigned int&)gNewHeight);
	gCurWidth = gNewWidth;
	gCurHeight = gNewHeight;

	RenderTargetCreateDescriptor rtDesc = {};
	rtDesc.width = (u16)gCurWidth;
	rtDesc.height = (u16)gCurHeight;

	rtDesc.colorFormat[0] = ll::graphics::Format::BGRA8_UNORM;
	rtDesc.numViews = 1;
	rtDesc.resizable = true;
	rtDesc.numSamples = 4;
	rtDesc.numExternalFramebuffers = (u8)gNumFrames;
	gSwapchainRenderTargetHandle = gDevice->CreateRenderTarget(rtDesc);

	IDXGISwapChain1* swapchain1 = nullptr;

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

	ll::os::gOS->CreateSwapchainForD3D12(&sdesc, &swapchain1);

	D3D_CALL(swapchain1->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&gSwapChain));
	swapchain1->Release();

	// Create backbuffer resources
	gBBFrames.resize(gNumFrames);
	std::vector<ID3D12Resource*> backbufferResources(gNumFrames);

	for (uint32_t i = 0; i < gNumFrames; ++i)
	{
		BackbufferResourceInfo& bb = gBBFrames[i];		
		bb.frameIndex = i;
		gSwapChain->GetBuffer(i, IID_PPV_ARGS(&backbufferResources[i]));
	}

	gSwapchainRenderTargetHandle.SetFramebuffers((void**)backbufferResources.data(), (u8)backbufferResources.size());
	
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

	// Release MSAA resources
	if (gMSAARenderTarget)
	{
		gMSAARenderTarget->Release();
		gMSAARenderTarget = nullptr;
	}
	if (gMSAARenderTargetAllocation)
	{
		gMSAARenderTargetAllocation->Release();
		gMSAARenderTargetAllocation = nullptr;
	}

	// Release depth-stencil resources
	if (gDepthStencilBuffer)
	{
		gDepthStencilBuffer->Release();
		gDepthStencilBuffer = nullptr;
	}
	if (gDepthStencilAllocation)
	{
		gDepthStencilAllocation->Release();
		gDepthStencilAllocation = nullptr;
	}

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

ll:os::gOS->GetActualWindowSize((unsigned int&)gNewWidth, (unsigned int&)gNewHeight);

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

		// Resize depth stencil buffer
		{
			D3D_CALL(gDepthStencilBuffer->Release());
			gDepthStencilBuffer = nullptr;

			D3D12_RESOURCE_DESC depthDesc = {};
			depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			depthDesc.Width = gCurWidth;
			depthDesc.Height = gCurHeight;
			depthDesc.MipLevels = 1;
			depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
			depthDesc.SampleDesc.Count = 1;
			depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			depthDesc.DepthOrArraySize = 1;

			D3D12MA::ALLOCATION_DESC depthAllocDesc = {};
			depthAllocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
			depthAllocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

			D3D_CALL(gAllocator->CreateResource(
				&depthAllocDesc,
				&depthDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				nullptr,
				&gDepthStencilAllocation,
				IID_PPV_ARGS(&gDepthStencilBuffer)
			));

			gDepthStencilView = CD3DX12_CPU_DESCRIPTOR_HANDLE(gDescriptorsHeapDSV->GetCPUDescriptorHandleForHeapStart());

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

			gD3DDevice->CreateDepthStencilView(gDepthStencilBuffer, &dsvDesc, gDepthStencilView);
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

	frame.commandList->ClearDepthStencilView(gDepthStencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	if (gMSAASampleCount > 1)
	{
		frame.commandList->ClearRenderTargetView(gMSAARenderTargetView, clearColorD3D, 0, nullptr);
		frame.commandList->OMSetRenderTargets(1, &gMSAARenderTargetView, FALSE, &gDepthStencilView);
	}
	else
	{
		frame.commandList->ClearRenderTargetView(bbResource.bbRTV, clearColorD3D, 0, nullptr);
		frame.commandList->OMSetRenderTargets(1, &bbResource.bbRTV, FALSE, &gDepthStencilView);
	}
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

	// Resolve MSAA to backbuffer if MSAA is enabled
	if (gMSAASampleCount > 1)
	{
		// Transition MSAA render target to resolve source
		CD3DX12_RESOURCE_BARRIER barriers[2] = {
			CD3DX12_RESOURCE_BARRIER::Transition(
				gMSAARenderTarget,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(
				bbResource.bbResource,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_RESOLVE_DEST)
		};
		frame.commandList->ResourceBarrier(2, barriers);

		// Resolve MSAA to backbuffer
		frame.commandList->ResolveSubresource(
			bbResource.bbResource, 0,
			gMSAARenderTarget, 0,
			gD3DSwapChainFormat
		);

		// Transition backbuffer to present
		CD3DX12_RESOURCE_BARRIER presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bbResource.bbResource,
			D3D12_RESOURCE_STATE_RESOLVE_DEST,
			D3D12_RESOURCE_STATE_PRESENT);
		frame.commandList->ResourceBarrier(1, &presentBarrier);

		// Transition MSAA render target back to render target for next frame
		CD3DX12_RESOURCE_BARRIER msaaBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			gMSAARenderTarget,
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		frame.commandList->ResourceBarrier(1, &msaaBarrier);
	}
	else
	{
		// Transition back to present (non-MSAA path)
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bbResource.bbResource,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
		frame.commandList->ResourceBarrier(1, &barrier);
	}

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

u32 Swapchain::GetCurrentFrameIndex() const
{
	return gFrameRingCurrent;
}
#endif