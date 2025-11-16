#include "d3d12_graphics.h"

#if defined(MERCURY_LL_GRAPHICS_D3D12)

using namespace mercury;
using namespace mercury::ll::graphics;

bool mercury::ll::graphics::IsYFlipped()
{
	return true;
}
#include "mercury_memory.h"
#include "mercury_log.h"
#include "mercury_application.h"
#include "mercury_utils.h"

#include "../../../imgui/imgui_impl.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#pragma comment(lib, "d3dcompiler.lib")  // For D3DReflect
#pragma comment(lib, "dxguid.lib")  
#pragma comment(lib, "dxcompiler.lib")

#include <d3d12shader.h>
#include <d3dcompiler.h>

#include <dxcapi.h>

#include <d3d12shader.h>

#include <wrl/client.h>

// Include D3D12 Memory Allocator implementation in this translation unit
#include "../../../../engine/third_party/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp"

IDXGIFactory4* gD3DFactory = nullptr;
ID3D12Debug1* gDebugController = nullptr;
IDXGIAdapter1* gD3DAdapter = nullptr;
ID3D12Device* gD3DDevice = nullptr;
ID3D12DebugDevice* gD3DDebugDevice = nullptr;
ID3D12CommandQueue* gD3DCommandQueue = nullptr;
ID3D12CommandAllocator* gD3DCommandAllocator = nullptr;
ID3D12DescriptorHeap* gDescriptorsHeapRTV = nullptr;
ID3D12DescriptorHeap* gDescriptorsHeapDSV = nullptr;
ID3D12DescriptorHeap* gDescriptorsHeapSRV = nullptr;

D3D12MA::Allocator* gAllocator = nullptr;
DXGI_FORMAT gD3DSwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

// Global storage for shaders, signatures, and PSOs
std::vector<CD3DX12_SHADER_BYTECODE> gAllShaders;

std::vector<PSOInfo> gAllPSOs;

std::vector<ID3D12Resource*> gAllBuffers;
std::vector<BufferInfo> gAllBuffersMeta;

std::vector<ParameterBlockDescriptor> gAllParameterBlocks;

struct TextureInfo
{
	ID3D12Resource* resource = nullptr;
	D3D12MA::Allocation* allocation = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = {};
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = {};
};

std::vector<TextureInfo> gAllTextures;

Device* gDevice = nullptr;
Instance* gInstance = nullptr;
Adapter* gAdapter = nullptr;
Swapchain* gSwapchain = nullptr;

mercury::memory::ReservedAllocator* memory::gGraphicsMemoryAllocator = nullptr;

const char* ll::graphics::GetBackendName()
{
	static const char* backendName = "D3D12";
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

	const auto& renderCfg = Application::GetCurrentApplication()->GetConfig().graphics;

	UINT dxgiFactoryFlags = 0;

	if (renderCfg.enableValidationLayers)
	{
		ID3D12Debug* dc;
		D3D_CALL(D3D12GetDebugInterface(IID_PPV_ARGS(&dc)));
		D3D_CALL(dc->QueryInterface(IID_PPV_ARGS(&gDebugController)));
		gDebugController->EnableDebugLayer();
		gDebugController->SetEnableGPUBasedValidation(true);
		gDebugController->SetEnableSynchronizedCommandQueueValidation(true);
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

	auto selectedAdapterID = renderCfg.explicitAdapterIndex == 255 ? 0 : renderCfg.explicitAdapterIndex;

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

void* Instance::GetNativeHandle()
{
	return nullptr;
}

void Instance::AcquireAdapter(const AdapterSelectorInfo& selector_info)
{
	if (gAdapter == nullptr)
		gAdapter = new Adapter();
}

void* Adapter::GetNativeHandle()
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

	const auto& renderCfg = Application::GetCurrentApplication()->GetConfig().graphics;

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

	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = 4096;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		D3D_CALL(gD3DDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gDescriptorsHeapSRV)));
		gDescriptorsHeapSRV->SetName(L"SRV Heap");
	}
	// Initialize D3D12 Memory Allocator
	D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
	allocatorDesc.pDevice = gD3DDevice;
	allocatorDesc.pAdapter = gD3DAdapter;
	allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED |
		D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED | D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED;
	
	HRESULT hr = D3D12MA::CreateAllocator(&allocatorDesc, &gAllocator);
	if (FAILED(hr))
	{
		MLOG_ERROR(u8"Failed to create D3D12MA::Allocator: HRESULT=0x%08X", hr);
		gAllocator = nullptr;
	}
	else
	{
		MLOG_DEBUG(u8"D3D12MA::Allocator created successfully");
	}
}

void Device::Shutdown()
{
	MLOG_DEBUG(u8"Shutdown Device (D3D12)");

	if (gDescriptorsHeapSRV) gDescriptorsHeapSRV->Release();
	gDescriptorsHeapSRV = nullptr;
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

static UINT currentSRVOffset = 0;

void AllocateSrvDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
{
	static UINT descriptorSize = gD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	*out_cpu_desc_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		info->SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		currentSRVOffset,
		descriptorSize);
	*out_gpu_desc_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
		info->SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(),
		currentSRVOffset,
		descriptorSize);
	currentSRVOffset++;
}

void FreeSrvDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle)
{
	// No-op for now
}

void Device::ImguiInitialize()
{


	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.CommandQueue = gD3DCommandQueue;
	init_info.Device = gD3DDevice;
	init_info.SrvDescriptorHeap = gDescriptorsHeapSRV;
	init_info.NumFramesInFlight = 3;
	init_info.RTVFormat = gD3DSwapChainFormat;

	init_info.SrvDescriptorAllocFn = &AllocateSrvDescriptor;
	init_info.SrvDescriptorFreeFn = &FreeSrvDescriptor;
	//init_info.SrvDescriptorAllocFn
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

void* Device::GetNativeHandle()
{
	return gD3DDevice;
}

void* Swapchain::GetNativeHandle()
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
	void* windowHandle = os::gOS->GetCurrentNativeWindowHandle();

	ll::os::gOS->GetActualWindowSize((unsigned int&)gNewWidth, (unsigned int&)gNewHeight);
	gCurWidth = gNewWidth;
	gCurHeight = gNewHeight;

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

u32 Swapchain::GetCurrentFrameIndex() const
{
	return gFrameRingCurrent;
}

void TimelineSemaphore::WaitUntil(mercury::u64 value, mercury::u64 timeout)
{
}

void TimelineSemaphore::SetDebugName(const char* utf8_name)
{
}

void TimelineSemaphore::Destroy()
{
	nativePtr = nullptr;
}

void RenderPass::SetDebugName(const char* utf8_name)
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

void CommandList::SetDebugName(const char* utf8_name)
{
}

void CommandList::Destroy()
{
	nativePtr = nullptr;
}

CommandList CommandPool::AllocateCommandList()
{
	CommandList result = {};
	result.nativePtr = nullptr;
	// In D3D12, command lists are managed per-frame, not via pools
	return result;
}

void CommandPool::SetDebugName(const char* utf8_name)
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
	cmdListD3D12->SetDescriptorHeaps(1, &gDescriptorsHeapSRV);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdListD3D12);
}

void CommandList::SetPSO(PsoHandle psoID)
{
	if (currentPsoID == psoID)
		return;

	currentPsoID = psoID;

	auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);

	auto const& p = gAllPSOs[psoID.handle];

	cmdListD3D12->SetPipelineState(p.pso);
	cmdListD3D12->SetGraphicsRootSignature(p.rootSignature);
	cmdListD3D12->IASetPrimitiveTopology(p.primitiveTopology);
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

D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyTypeFromMercuryTopology(mercury::ll::graphics::PrimitiveTopology topology)
{
	switch (topology)
	{
	case mercury::ll::graphics::PrimitiveTopology::TriangleList:
	case mercury::ll::graphics::PrimitiveTopology::TriangleStrip:	
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case mercury::ll::graphics::PrimitiveTopology::LineList:
	case mercury::ll::graphics::PrimitiveTopology::LineStrip:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case mercury::ll::graphics::PrimitiveTopology::PointList:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	default:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	}
}
D3D_PRIMITIVE_TOPOLOGY PrimitiveTopologyFromMercuryPrimitiveTopology(mercury::ll::graphics::PrimitiveTopology topology)
{
	switch (topology)
	{
	case mercury::ll::graphics::PrimitiveTopology::TriangleList:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case mercury::ll::graphics::PrimitiveTopology::TriangleStrip:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	case mercury::ll::graphics::PrimitiveTopology::LineList:
		return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	case mercury::ll::graphics::PrimitiveTopology::LineStrip:
		return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case mercury::ll::graphics::PrimitiveTopology::PointList:
		return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	default:
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}
}

PsoHandle Device::CreateRasterizePipeline(const mercury::ll::graphics::RasterizePipelineDescriptor& desc)
{
	using Microsoft::WRL::ComPtr;

	// TODO: Implement full D3D12 rasterize pipeline creation
	PsoHandle result;
	result.handle = static_cast<u32>(gAllPSOs.size());

	auto& pso = gAllPSOs.emplace_back();

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;

	std::vector<CD3DX12_ROOT_PARAMETER> rootParameters;
	
	if (desc.vertexShader.isValid())
	{
		auto& vsBytecode = gAllShaders[desc.vertexShader.handle];

		// Validate bytecode before reflection
		if (vsBytecode.pShaderBytecode == nullptr || vsBytecode.BytecodeLength == 0)
		{
			MLOG_ERROR(u8"Vertex shader bytecode is null or empty!");
		}
		else
		{
			MLOG_DEBUG(u8"Reflecting DXIL vertex shader: %zu bytes at %p",
				vsBytecode.BytecodeLength, vsBytecode.pShaderBytecode);

			ComPtr<IDxcUtils> dxcUtils;
			HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));

			if (SUCCEEDED(hr))
			{
				// Create a blob wrapper around the shader bytecode
				ComPtr<IDxcBlobEncoding> shaderBlob;
				hr = dxcUtils->CreateBlob(
					vsBytecode.pShaderBytecode,
					static_cast<UINT32>(vsBytecode.BytecodeLength),
					CP_ACP,
					&shaderBlob
				);

				if (SUCCEEDED(hr))
				{
					ComPtr<IDxcContainerReflection> containerReflection;
					hr = DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&containerReflection));

					if (SUCCEEDED(hr))
					{
						hr = containerReflection->Load(shaderBlob.Get());

						if (SUCCEEDED(hr))
						{
							UINT32 partIndex;
							hr = containerReflection->FindFirstPartKind(DXC_PART_DXIL, &partIndex);

							if (SUCCEEDED(hr))
							{
								ComPtr<ID3D12ShaderReflection> reflection;
								hr = containerReflection->GetPartReflection(partIndex, IID_PPV_ARGS(&reflection));

								if (SUCCEEDED(hr))
								{
									D3D12_SHADER_DESC shaderDesc;
									reflection->GetDesc(&shaderDesc);

									MLOG_DEBUG(u8"DXIL Vertex Shader has %u bound resources:", shaderDesc.BoundResources);

									for (UINT i = 0; i < shaderDesc.BoundResources; ++i)
									{
										D3D12_SHADER_INPUT_BIND_DESC bindDesc;
										reflection->GetResourceBindingDesc(i, &bindDesc);

										MLOG_DEBUG(u8"  Resource '%s': Type=%d, BindPoint=%u, Space=%u, BindCount=%u",
											bindDesc.Name,
											bindDesc.Type,
											bindDesc.BindPoint,
											bindDesc.Space,
											bindDesc.BindCount);
									}
								}
								else
								{
									MLOG_ERROR(u8"Failed to get part reflection: 0x%08X", hr);
								}
							}
							else
							{
								MLOG_ERROR(u8"Failed to find DXIL part: 0x%08X", hr);
							}
						}
						else
						{
							MLOG_ERROR(u8"Failed to load container reflection: 0x%08X", hr);
						}
					}
				}
				else
				{
					MLOG_ERROR(u8"Failed to create DXC blob: 0x%08X", hr);
				}
			}
			else
			{
				MLOG_ERROR(u8"Failed to create DxcUtils: 0x%08X", hr);
			}
		}
	}

	if (desc.fragmentShader.isValid())
	{
		auto& fsBytecode = gAllShaders[desc.fragmentShader.handle];

		// Validate bytecode before reflection
		if (fsBytecode.pShaderBytecode == nullptr || fsBytecode.BytecodeLength == 0)
		{
			MLOG_ERROR(u8"Vertex shader bytecode is null or empty!");
		}
		else
		{
			MLOG_DEBUG(u8"Reflecting DXIL vertex shader: %zu bytes at %p",
				fsBytecode.BytecodeLength, fsBytecode.pShaderBytecode);

			ComPtr<IDxcUtils> dxcUtils;
			HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));

			if (SUCCEEDED(hr))
			{
				// Create a blob wrapper around the shader bytecode
				ComPtr<IDxcBlobEncoding> shaderBlob;
				hr = dxcUtils->CreateBlob(
					fsBytecode.pShaderBytecode,
					static_cast<UINT32>(fsBytecode.BytecodeLength),
					CP_ACP,
					&shaderBlob
				);

				if (SUCCEEDED(hr))
				{
					ComPtr<IDxcContainerReflection> containerReflection;
					hr = DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&containerReflection));

					if (SUCCEEDED(hr))
					{
						hr = containerReflection->Load(shaderBlob.Get());

						if (SUCCEEDED(hr))
						{
							UINT32 partIndex;
							hr = containerReflection->FindFirstPartKind(DXC_PART_DXIL, &partIndex);

							if (SUCCEEDED(hr))
							{
								ComPtr<ID3D12ShaderReflection> reflection;
								hr = containerReflection->GetPartReflection(partIndex, IID_PPV_ARGS(&reflection));

								if (SUCCEEDED(hr))
								{
									D3D12_SHADER_DESC shaderDesc;
									reflection->GetDesc(&shaderDesc);

									MLOG_DEBUG(u8"DXIL Fragment Shader has %u bound resources:", shaderDesc.BoundResources);

									for (UINT i = 0; i < shaderDesc.BoundResources; ++i)
									{
										D3D12_SHADER_INPUT_BIND_DESC bindDesc;
										reflection->GetResourceBindingDesc(i, &bindDesc);

										MLOG_DEBUG(u8"  Resource '%s': Type=%d, BindPoint=%u, Space=%u, BindCount=%u",
											bindDesc.Name,
											bindDesc.Type,
											bindDesc.BindPoint,
											bindDesc.Space,
											bindDesc.BindCount);
									}
								}
								else
								{
									MLOG_ERROR(u8"Failed to get part reflection: 0x%08X", hr);
								}
							}
							else
							{
								MLOG_ERROR(u8"Failed to find DXIL part: 0x%08X", hr);
							}
						}
						else
						{
							MLOG_ERROR(u8"Failed to load container reflection: 0x%08X", hr);
						}
					}
				}
				else
				{
					MLOG_ERROR(u8"Failed to create DXC blob: 0x%08X", hr);
				}
			}
			else
			{
				MLOG_ERROR(u8"Failed to create DxcUtils: 0x%08X", hr);
			}
		}
	}

	if (desc.pushConstantSize > 0)
	{
		CD3DX12_ROOT_PARAMETER rootParam;
		rootParam.InitAsConstants(desc.pushConstantSize / 4, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters.push_back(rootParam);

		pso.rootParameterRootConstantIndex = 0;
	}

	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;

	for (int i = 0; i < 3; ++i)
	{
		auto& bs_layout = desc.bindingSetLayouts[i];

		pso.setOffsets[i] = static_cast<u32>(rootParameters.size());

		for (int j = 0; j < bs_layout.allSlots.size(); ++j)
		{
			const auto& slot = bs_layout.allSlots[j];

			if (slot.resourceType == ShaderResourceType::UniformBuffer)
			{
				CD3DX12_ROOT_PARAMETER rootParam;
				rootParam.InitAsConstantBufferView(j, i + (desc.pushConstantSize > 0), D3D12_SHADER_VISIBILITY_ALL);
				rootParameters.push_back(rootParam);
			}

			if (slot.resourceType == ShaderResourceType::SampledImage2D)
			{
				CD3DX12_DESCRIPTOR_RANGE srvRange;
				srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, j, i + (desc.pushConstantSize > 0));
				CD3DX12_ROOT_PARAMETER rootParam;
				rootParam.InitAsDescriptorTable(1, &srvRange);
				rootParameters.push_back(rootParam);
			}
		}
	}

	// Add static sampler if any textures are present
	bool hasTextures = false;
	for (int i = 0; i < 3; ++i)
	{
		for (const auto& slot : desc.bindingSetLayouts[i].allSlots)
		{
			if (slot.resourceType == ShaderResourceType::SampledImage2D)
			{
				hasTextures = true;
				break;
			}
		}
		if (hasTextures) break;
	}
	if (hasTextures)
	{
		auto& sdesc = staticSamplers.emplace_back();
		sdesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sdesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sdesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sdesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sdesc.MipLODBias = 0.0f;
		sdesc.MaxAnisotropy = 1;
		sdesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		sdesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		sdesc.MinLOD = 0.0f;
		sdesc.MaxLOD = D3D12_FLOAT32_MAX;
		sdesc.ShaderRegister = 0; // s0
		sdesc.RegisterSpace = 2;
		sdesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	rootSignatureDesc.Init(static_cast<UINT>(rootParameters.size()), rootParameters.data(), staticSamplers.size(), staticSamplers.data(), rootSignatureFlags);

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;
	D3D_CALL(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));


	D3D_CALL(gD3DDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&pso.rootSignature)));

	// Release the blob as it's no longer needed
	signature->Release();
	if (error) error->Release();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};


	psoDesc.pRootSignature = pso.rootSignature;

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
	psoDesc.PrimitiveTopologyType = PrimitiveTopologyTypeFromMercuryTopology(desc.primitiveTopology);
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	D3D_CALL(gD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso.pso)));

	pso.primitiveTopology = PrimitiveTopologyFromMercuryPrimitiveTopology(desc.primitiveTopology);
	
	return result;
}

void Device::UpdatePipelineState(PsoHandle psoID, const mercury::ll::graphics::RasterizePipelineDescriptor& desc)
{
	// TODO: Implement D3D12 pipeline state update
}

void Device::DestroyRasterizePipeline(PsoHandle psoID)
{
	gAllPSOs[psoID.handle].pso->Release();
	gAllPSOs[psoID.handle].pso = nullptr;

	gAllPSOs[psoID.handle].rootSignature->Release();
	gAllPSOs[psoID.handle].rootSignature = nullptr;
}

CommandPool Device::CreateCommandPool(QueueType queue_type)
{
	CommandPool pool = {};
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

void CommandList::PushConstants(const void* data, size_t size)
{
	auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);

	cmdListD3D12->SetGraphicsRoot32BitConstants(gAllPSOs[currentPsoID.handle].rootParameterRootConstantIndex, static_cast<UINT>(size / 4), data, 0);
}

BufferHandle Device::CreateBuffer(const BufferDescriptor& desc)
{
	BufferHandle result;
	result.handle = static_cast<u32>(gAllBuffers.size());

	auto size = mercury::utils::math::alignUp(desc.size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	// Validate allocator is initialized
	if (!gAllocator)
	{
		MLOG_ERROR(u8"D3D12MA::Allocator is not initialized!");
		result.handle = BufferHandle::InvalidValue;
		gAllBuffersMeta.push_back(BufferInfo{});
		gAllBuffers.push_back(nullptr);
		return result;
	}

	// Validate size
	if (size == 0)
	{
		MLOG_ERROR(u8"Cannot create buffer with size 0");
		result.handle = BufferHandle::InvalidValue;
		gAllBuffersMeta.push_back(BufferInfo{});
		gAllBuffers.push_back(nullptr);
		return result;
	}

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Alignment = 0;
	bufferDesc.Width = size;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.SampleDesc.Quality = 0;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12MA::ALLOCATION_DESC allocDesc = {};
	allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
	allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;

	BufferInfo bufferInfo = {};
	ID3D12Resource* bufferResource = nullptr;
	bufferInfo.size = size;

	HRESULT hr = gAllocator->CreateResource(
		&allocDesc,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ | D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_COPY_SOURCE,
		nullptr,
		&bufferInfo.allocation,
		IID_PPV_ARGS(&bufferResource)
	);

	if (FAILED(hr))
	{
		MLOG_ERROR(u8"Failed to create D3D12 buffer of size %zu bytes: HRESULT=0x%08X", size, hr);
		result.handle = BufferHandle::InvalidValue;
		gAllBuffersMeta.push_back(BufferInfo{});
		gAllBuffers.push_back(nullptr);
		return result;
	}

	if (desc.initialData)
	{
		void* mappedData = nullptr;
		D3D12_RANGE readRange = { 0, 0 };
		hr = bufferResource->Map(0, &readRange, &mappedData);
		memcpy(mappedData, desc.initialData, desc.size);
		D3D12_RANGE writtenRange = { 0, desc.size };
		bufferResource->Unmap(0, &writtenRange);
	}

	MLOG_DEBUG(u8"Created D3D12 buffer: handle=%u, size=%zu bytes", result.handle, size);

	gAllBuffersMeta.push_back(bufferInfo);
	gAllBuffers.push_back(bufferResource);

	return result;
}

void Device::DestroyBuffer(BufferHandle bufferID)
{
	if (!bufferID.isValid() || bufferID.handle >= gAllBuffers.size())
	{
		MLOG_WARNING(u8"Attempting to destroy invalid buffer handle: %u", bufferID.handle);
		return;
	}

	BufferInfo& bufferInfo = gAllBuffersMeta[bufferID.handle];
	ID3D12Resource* bufferResource = gAllBuffers[bufferID.handle];

	if (bufferResource)
	{
		bufferResource->Release();
	}

	if (bufferInfo.allocation)
	{
		bufferInfo.allocation->Release();
		bufferInfo.allocation = nullptr;
	}

	bufferInfo.size = 0;

	MLOG_DEBUG(u8"Destroyed D3D12 buffer: handle=%u", bufferID.handle);
}

void Device::UpdateBuffer(BufferHandle bufferID, const void* data, size_t size, size_t offset)
{
	if (!bufferID.isValid() || bufferID.handle >= gAllBuffers.size())
	{
		MLOG_ERROR(u8"Attempting to update invalid buffer handle: %u", bufferID.handle);
		return;
	}

	BufferInfo& bufferInfo = gAllBuffersMeta[bufferID.handle];
	ID3D12Resource* bufferResource = gAllBuffers[bufferID.handle];

	if (!bufferResource)
	{
		MLOG_ERROR(u8"Buffer resource is null for handle: %u", bufferID.handle);
		return;
	}

	// Validate bounds
	if (offset + size > bufferInfo.size)
	{
		MLOG_ERROR(u8"Buffer update out of bounds: handle=%u, offset=%zu, size=%zu, buffer_size=%zu",
			bufferID.handle, offset, size, bufferInfo.size);
		return;
	}

	void* mappedData = nullptr;
	D3D12_RANGE readRange = { 0, 0 };
	HRESULT hr = bufferResource->Map(0, &readRange, &mappedData);
	memcpy(static_cast<u8*>(mappedData) + offset, data, size);
	D3D12_RANGE writtenRange = { offset, offset + size };
	bufferResource->Unmap(0, &writtenRange);
}

ParameterBlockLayoutHandle Device::CreateParameterBlockLayout(const BindingSetLayoutDescriptor& layoutDesc, int setIndex)
{
	std::vector<CD3DX12_ROOT_PARAMETER> rootParameters;

	for (int i = 0; i < layoutDesc.allSlots.size(); ++i)
	{
		CD3DX12_ROOT_PARAMETER rootParam2;
		rootParam2.InitAsConstantBufferView(i, setIndex, D3D12_SHADER_VISIBILITY_ALL);
		rootParameters.push_back(rootParam2);
	}

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(rootParameters.size(), rootParameters.data(), 0, nullptr, rootSignatureFlags);

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;
	D3D_CALL(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));

	// Create the actual root signature from the blob
	ID3D12RootSignature* rootSignature = nullptr;
	D3D_CALL(gD3DDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

	// Release the blob as it's no longer needed
	signature->Release();
	if (error) error->Release();

	auto &pso = gAllPSOs.emplace_back();
	pso.rootSignature = rootSignature;

	ParameterBlockLayoutHandle result;
	result.handle = static_cast<u32>(gAllPSOs.size() - 1);
	return result;
}

void Device::DestroyParameterBlockLayout(ParameterBlockLayoutHandle layoutID)
{
	gAllPSOs[layoutID.handle].rootSignature->Release();	
}

void  CommandList::SetParameterBlockLayout(u8 set_index, ParameterBlockLayoutHandle layoutID)
{
	auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);
	cmdListD3D12->SetGraphicsRootSignature(gAllPSOs[layoutID.handle].rootSignature);
}

//void CommandList::SetUniformBuffer(u8 binding_slot, BufferHandle bufferID, size_t offset, size_t size)
//{
//	auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);
//	auto bufferResource = gAllBuffers[bufferID.handle];
//	cmdListD3D12->SetGraphicsRootConstantBufferView(binding_slot, bufferResource->GetGPUVirtualAddress());
//}

void CommandList::SetParameterBlock(u8 setIndex, ParameterBlockHandle parameterBlockID)
{
	u32 slotIndex = 0;
	auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);
	const auto& pbDesc = gAllParameterBlocks[parameterBlockID.handle];
	int slotStartIndex = gAllPSOs[currentPsoID.handle].setOffsets[setIndex];

	// Set descriptor heap if there are textures
	bool hasTextures = false;
	for (const auto& res : pbDesc.resources)
	{
		if (std::holds_alternative<ParameterResourceTexture>(res))
		{
			hasTextures = true;
			break;
		}
	}
	if (hasTextures)
	{
		cmdListD3D12->SetDescriptorHeaps(1, &gDescriptorsHeapSRV);
	}

	for (const auto& res : pbDesc.resources)
	{
		std::visit([&](auto&& arg)
			{
				using T = std::decay_t<decltype(arg)>;

				if constexpr (std::is_same_v<T, ParameterResourceBuffer>)
				{
					cmdListD3D12->SetGraphicsRootConstantBufferView(slotStartIndex + slotIndex, gAllBuffers[arg.buffer.handle]->GetGPUVirtualAddress());
				}
				else if constexpr (std::is_same_v<T, ParameterResourceTexture>)
				{
					cmdListD3D12->SetGraphicsRootDescriptorTable(slotStartIndex + slotIndex, gAllTextures[arg.texture.handle].srvGpuHandle);
				}
				else if constexpr (std::is_same_v<T, ParameterResourceRWImage>)
				{
					// TODO: Fill VkDescriptorImageInfo for storage image
					MLOG_WARNING(u8"ParameterResourceRWImage not yet implemented in UpdateParameterBlock");
				}
				else if constexpr (std::is_same_v<T, ParameterResourceEmpty>)
				{
					// Intentionally empty slot – skip writing
				}
			}, res);

		slotIndex++;
	}
}

ParameterBlockHandle Device::CreateParameterBlock(const ParameterBlockLayoutHandle& layoutID)
{
	auto& ds = gAllParameterBlocks.emplace_back();

	return ParameterBlockHandle{ static_cast<u32>(gAllParameterBlocks.size() - 1) };
}

void Device::UpdateParameterBlock(ParameterBlockHandle parameterBlockID, const ParameterBlockDescriptor& pbDesc)
{
	gAllParameterBlocks[parameterBlockID.handle] = pbDesc;

	//std::vector<VkWriteDescriptorSet> writes;
	//std::vector<VkDescriptorBufferInfo> bufferInfos;
	//std::vector<VkDescriptorImageInfo>  imageInfos;

	//u32 slotIndex = 0;
	//writes.reserve(pbDesc.resources.size());
	//bufferInfos.reserve(pbDesc.resources.size());

	//for (const auto& res : pbDesc.resources)
	//{
	//	std::visit([&](auto&& arg)
	//		{
	//			using T = std::decay_t<decltype(arg)>;

	//			if constexpr (std::is_same_v<T, ParameterResourceBuffer>)
	//			{
	//				VkDescriptorBufferInfo bi{};
	//				bi.buffer = gAllBuffers[arg.buffer.handle];
	//				bi.offset = static_cast<VkDeviceSize>(arg.offset);
	//				bi.range = (arg.size == SIZE_MAX) ? VK_WHOLE_SIZE : static_cast<VkDeviceSize>(arg.size);

	//				bufferInfos.push_back(bi);

	//				VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	//				w.dstSet = gAllDescriptorSets[parameterBlockID.handle];
	//				w.dstBinding = slotIndex;
	//				w.descriptorCount = 1;
	//				// If you cached per-slot descriptor types when creating the layout, use that here.
	//				// Without that metadata, default to UNIFORM_BUFFER for buffer resources:
	//				w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	//				w.pBufferInfo = &bufferInfos.back();

	//				writes.push_back(w);
	//			}
	//			else if constexpr (std::is_same_v<T, ParameterResourceTexture>)
	//			{
	//				// TODO: Fill VkDescriptorImageInfo (imageView, sampler if combined, layout)
	//				// imageInfos.push_back(imgInfo);
	//				// VkWriteDescriptorSet w{...}; w.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE (or COMBINED_IMAGE_SAMPLER)
	//				// w.pImageInfo = &imageInfos.back(); writes.push_back(w);
	//				MLOG_WARNING(u8"ParameterResourceTexture not yet implemented in UpdateParameterBlock");
	//			}
	//			else if constexpr (std::is_same_v<T, ParameterResourceRWImage>)
	//			{
	//				// TODO: Fill VkDescriptorImageInfo for storage image
	//				MLOG_WARNING(u8"ParameterResourceRWImage not yet implemented in UpdateParameterBlock");
	//			}
	//			else if constexpr (std::is_same_v<T, ParameterResourceEmpty>)
	//			{
	//				// Intentionally empty slot – skip writing
	//			}
	//		}, res);

	//	slotIndex++;
	//}

	//if (!writes.empty())
	//{
	//	vkUpdateDescriptorSets(gVKDevice, static_cast<u32>(writes.size()), writes.data(), 0, nullptr);
	//}
}

TextureHandle Device::CreateTexture(const TextureDescriptor& desc)
{
	TextureInfo texInfo = {};

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Alignment = 0;
	textureDesc.Width = desc.width;
	textureDesc.Height = desc.height;
	textureDesc.DepthOrArraySize = desc.depth;
	textureDesc.MipLevels = desc.mipLevels;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12MA::ALLOCATION_DESC allocDesc = {};
	allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
	allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;


	HRESULT hr = gAllocator->CreateResource(
		&allocDesc,
		&textureDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		&texInfo.allocation,
		IID_PPV_ARGS(&texInfo.resource)
	);

	if (desc.initialData)
	{
		// Calculate data size (assuming RGBA8)
		size_t dataSize = desc.width * desc.height * 4;

		// Create staging buffer
		D3D12_RESOURCE_DESC stagingDesc = {};
		stagingDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		stagingDesc.Alignment = 0;
		stagingDesc.Width = dataSize;
		stagingDesc.Height = 1;
		stagingDesc.DepthOrArraySize = 1;
		stagingDesc.MipLevels = 1;
		stagingDesc.Format = DXGI_FORMAT_UNKNOWN;
		stagingDesc.SampleDesc.Count = 1;
		stagingDesc.SampleDesc.Quality = 0;
		stagingDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		stagingDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12MA::ALLOCATION_DESC stagingAllocDesc = {};
		stagingAllocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
		stagingAllocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;

		D3D12MA::Allocation* stagingAllocation = nullptr;
		ID3D12Resource* stagingBuffer = nullptr;
		HRESULT hr = gAllocator->CreateResource(
			&stagingAllocDesc,
			&stagingDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			&stagingAllocation,
			IID_PPV_ARGS(&stagingBuffer)
		);
		if (FAILED(hr))
		{
			MLOG_ERROR(u8"Failed to create staging buffer for texture upload: HRESULT=0x%08X", hr);
		}
		else
		{
			// Map and copy data
			void* mappedData = nullptr;
			D3D12_RANGE readRange = { 0, 0 };
			hr = stagingBuffer->Map(0, &readRange, &mappedData);
			if (SUCCEEDED(hr))
			{
				memcpy(mappedData, desc.initialData, dataSize);
				D3D12_RANGE writtenRange = { 0, dataSize };
				stagingBuffer->Unmap(0, &writtenRange);

				// Create temporary command list for copy
				ID3D12CommandAllocator* tempAllocator = nullptr;
				ID3D12GraphicsCommandList* tempCmdList = nullptr;
				hr = gD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&tempAllocator));
				if (SUCCEEDED(hr))
				{
					hr = gD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, tempAllocator, nullptr, IID_PPV_ARGS(&tempCmdList));
					if (SUCCEEDED(hr))
					{
						tempCmdList->Close(); // Reset to open
						tempCmdList->Reset(tempAllocator, nullptr);

						// Transition texture to copy dest
						D3D12_RESOURCE_BARRIER barrier = {};
						barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
						barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
						barrier.Transition.pResource = texInfo.resource;
						barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
						barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
						barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
						tempCmdList->ResourceBarrier(1, &barrier);

						// Copy buffer to texture
						D3D12_TEXTURE_COPY_LOCATION src = {};
						src.pResource = stagingBuffer;
						src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
						src.PlacedFootprint.Offset = 0;
						src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
						src.PlacedFootprint.Footprint.Width = desc.width;
						src.PlacedFootprint.Footprint.Height = desc.height;
						src.PlacedFootprint.Footprint.Depth = 1;
						src.PlacedFootprint.Footprint.RowPitch = desc.width * 4;

						D3D12_TEXTURE_COPY_LOCATION dst = {};
						dst.pResource = texInfo.resource;
						dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
						dst.SubresourceIndex = 0;

						tempCmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

						// Transition back to generic read
						barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
						barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
						tempCmdList->ResourceBarrier(1, &barrier);

						// Close and execute
						tempCmdList->Close();
						ID3D12CommandList* cmdLists[] = { tempCmdList };
						gD3DCommandQueue->ExecuteCommandLists(1, cmdLists);

						// Wait for completion
						ID3D12Fence* tempFence = nullptr;
						gD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&tempFence));
						gD3DCommandQueue->Signal(tempFence, 1);
						if (tempFence->GetCompletedValue() < 1)
						{
							HANDLE event = CreateEvent(NULL, FALSE, FALSE, NULL);
							tempFence->SetEventOnCompletion(1, event);
							WaitForSingleObject(event, INFINITE);
							CloseHandle(event);
						}
						tempFence->Release();

						tempCmdList->Release();
					}
					tempAllocator->Release();
				}
			}
			stagingBuffer->Release();
			stagingAllocation->Release();
		}
	}

	auto descriptorSize = gD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	auto srvCpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		gDescriptorsHeapSRV->GetCPUDescriptorHandleForHeapStart(),
		currentSRVOffset,
		descriptorSize);
	auto srvGpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
		gDescriptorsHeapSRV->GetGPUDescriptorHandleForHeapStart(),
		currentSRVOffset,
		descriptorSize);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = desc.mipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	gD3DDevice->CreateShaderResourceView(texInfo.resource, &srvDesc, srvCpuHandle);

	texInfo.srvHandle = srvCpuHandle;
	texInfo.srvGpuHandle = srvGpuHandle;

	gAllTextures.push_back(texInfo);

	currentSRVOffset++;

	TextureHandle result;
	result.handle = static_cast<u32>(gAllTextures.size() - 1);
	return result;
}


void Device::DestroyParameterBlock(ParameterBlockHandle parameterBlockID)
{

}

u64 TextureHandle::CreateImguiTextureOpaqueHandle() const
{
	const auto& tex_data = &gAllTextures[handle];

	return (u64)(tex_data->srvGpuHandle.ptr);
}

#endif