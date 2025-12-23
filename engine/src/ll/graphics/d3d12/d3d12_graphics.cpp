#include "d3d12_graphics.h"

#if defined(MERCURY_LL_GRAPHICS_D3D12)

#include <winapifamily.h>


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

// Add utils for DXGI<->internal format conversion
#include "d3d12_utils.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <d3d12shader.h>

#include <dxcapi.h>

#include <d3d12shader.h>

#include <wrl/client.h>
#include "d3d12_graphics.h"
#include "d3d12_render_target.h"
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
UINT gCurrentSRVOffset = 0;

D3D12MA::Allocator* gAllocator = nullptr;
DXGI_FORMAT gD3DSwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
DXGI_FORMAT gD3DSwapChainDepthFormat = DXGI_FORMAT_D32_FLOAT;

// Global storage for shaders, signatures, and PSOs
std::vector<CD3DX12_SHADER_BYTECODE> gAllShaders;

std::vector<PSOInfo> gAllPSOs;

std::vector<ID3D12Resource*> gAllBuffers;
std::vector<BufferInfo> gAllBuffersMeta;

std::vector<ParameterBlockDescriptor> gAllParameterBlocks;

std::vector<TextureInfo> gAllTextures;

Device* gDevice = nullptr;
Instance* gInstance = nullptr;
Adapter* gAdapter = nullptr;
Swapchain* gSwapchain = nullptr;

mercury::memory::ReservedAllocator* memory::gGraphicsMemoryAllocator = nullptr;

std::vector<FrameData> gFrames;

u32 gFrameRingCurrent = 0;

// Add after the global swapchain variables (around line 58)
UINT gMSAASampleCount = 4; // Default to 4x MSAA
UINT gMSAAQuality = 0;

const char* ll::graphics::GetBackendName()
{
	static const char* backendName = "D3D12";
	return backendName;
}

#define RPC_NO_WINDOWS_H

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

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = 4096;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	D3D_CALL(gD3DDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gDescriptorsHeapSRV)));
	gDescriptorsHeapSRV->SetName(L"SRV Heap");
	
	InitRenderTargetHeaps();

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


void AllocateSrvDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
{
	static UINT descriptorSize = gD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	*out_cpu_desc_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		gDescriptorsHeapSRV->GetCPUDescriptorHandleForHeapStart(),
		gCurrentSRVOffset,
		descriptorSize);
	*out_gpu_desc_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
		gDescriptorsHeapSRV->GetGPUDescriptorHandleForHeapStart(),
		gCurrentSRVOffset,
		descriptorSize);
	gCurrentSRVOffset++;
}

void FreeSrvDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle)
{
	// No-op for now
}

void AllocateSrvDescriptorImgui(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
{
	AllocateSrvDescriptor(out_cpu_desc_handle, out_gpu_desc_handle);
}

void FreeSrvDescriptorImgui(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle)
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
	init_info.DSVFormat = gD3DSwapChainDepthFormat;
	init_info.SrvDescriptorAllocFn = &AllocateSrvDescriptorImgui;
	init_info.SrvDescriptorFreeFn = &FreeSrvDescriptorImgui;
	init_info.MSAASampleCount = gMSAASampleCount;
	init_info.MSAAQuality = gMSAAQuality;
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

void DebugShaderReflection(const mercury::ll::graphics::RasterizePipelineDescriptor& desc);

PsoHandle Device::CreateRasterizePipeline(const mercury::ll::graphics::RasterizePipelineDescriptor& desc)
{
	// TODO: Implement full D3D12 rasterize pipeline creation
	PsoHandle result;
	result.handle = static_cast<u32>(gAllPSOs.size());

	auto& pso = gAllPSOs.emplace_back();
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	if (!desc.verticesInputInfo.empty())
	{
		rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}

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

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;

	std::vector<CD3DX12_ROOT_PARAMETER> rootParameters;
	
	DebugShaderReflection(desc);

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

	psoDesc.pRootSignature = pso.rootSignature;

	// Release the blob as it's no longer needed
	signature->Release();
	if (error) error->Release();
	
	std::vector<D3D12_INPUT_ELEMENT_DESC> vtxInputElements;
	u32 vertexAttribOffset = 0;

	for (int i = 0; i < desc.verticesInputInfo.size(); ++i)
	{
		const auto& attr = desc.verticesInputInfo[i];
		D3D12_INPUT_ELEMENT_DESC elementDesc = {};
		elementDesc.SemanticName = attr.semanticName.c_str();
		elementDesc.SemanticIndex = 0;
		elementDesc.Format = ToDXGIFormat(attr.format);
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = vertexAttribOffset;
		elementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;
		elementDesc.SemanticIndex = attr.semanticIndex;
		vtxInputElements.push_back(elementDesc);

		const auto& finfo = ll::graphics::GetFormatInfo(desc.verticesInputInfo[i].format);
		vertexAttribOffset += finfo.blockSize;
	}
			
	psoDesc.InputLayout.pInputElementDescs = vtxInputElements.data();
	psoDesc.InputLayout.NumElements = static_cast<UINT>(vtxInputElements.size());

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.FrontCounterClockwise = true;
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthEnable = true;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	//psoDesc.DepthStencilState.DepthEnable = FALSE;
	//psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = PrimitiveTopologyTypeFromMercuryTopology(desc.primitiveTopology);
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = gD3DSwapChainFormat;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = gMSAASampleCount;
	psoDesc.SampleDesc.Quality = gMSAAQuality;

	
	//psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

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

BufferHandle Device::CreateBuffer(const BufferDescriptor& desc)
{
	BufferHandle result;
	result.handle = static_cast<u32>(gAllBuffers.size());

	auto size = desc.size;

	if (desc.type == BufferType::UniformBuffer)
	{
		size = mercury::utils::math::alignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}

	if (desc.type == BufferType::IndexBuffer || desc.type == BufferType::VertexBuffer)
	{
		size = mercury::utils::math::alignUp(size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	}
	

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

	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_COPY_SOURCE;

	if (desc.type == BufferType::UniformBuffer || desc.type == BufferType::VertexBuffer)
	{
		initialState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

	}
	else if (desc.type == BufferType::IndexBuffer)
	{
		initialState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
	}

	HRESULT hr = gAllocator->CreateResource(
		&allocDesc,
		&bufferDesc,
		initialState,
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
	bufferInfo.gpuAddress = bufferResource->GetGPUVirtualAddress();
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



ParameterBlockHandle Device::CreateParameterBlock(const ParameterBlockLayoutHandle& layoutID)
{
	auto& ds = gAllParameterBlocks.emplace_back();

	return ParameterBlockHandle{ static_cast<u32>(gAllParameterBlocks.size() - 1) };
}

void Device::UpdateParameterBlock(ParameterBlockHandle parameterBlockID, const ParameterBlockDescriptor& pbDesc)
{
	gAllParameterBlocks[parameterBlockID.handle] = pbDesc;
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
						
						MLOG_DEBUG(u8"Texture uploaded successfully: %ux%u", desc.width, desc.height);
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
		gCurrentSRVOffset,
		descriptorSize);
	auto srvGpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
		gDescriptorsHeapSRV->GetGPUDescriptorHandleForHeapStart(),
		gCurrentSRVOffset,
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

	gCurrentSRVOffset++;

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

void DebugShaderReflection(const mercury::ll::graphics::RasterizePipelineDescriptor& desc)
{
	if (desc.vertexShader.isValid())
	{
		auto& vsBytecode = gAllShaders[desc.vertexShader.handle];
		PrintShaderBytecodeInputs(vsBytecode);
	}
	if (desc.fragmentShader.isValid())
	{
		auto& fsBytecode = gAllShaders[desc.fragmentShader.handle];
		PrintShaderBytecodeInputs(fsBytecode);
	}
}
#endif////////////////////////////////////////////////////////////////////////////