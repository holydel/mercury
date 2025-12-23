#pragma once

#include "ll/graphics.h"

#if defined(MERCURY_LL_GRAPHICS_D3D12)


#include <directx/d3d12.h>
#include "directx/d3dx12.h"
#include <dxgi1_4.h>
#include <D3D12MemAlloc.h>
#include <system_error>
#include <ll/os.h>
#include <variant>

struct BufferInfo
{
	D3D12MA::Allocation* allocation = nullptr;
	size_t size = 0;
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
};

struct TextureInfo
{
	ID3D12Resource* resource = nullptr;
	D3D12MA::Allocation* allocation = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = {};
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = {};
};


extern IDXGIFactory4* gD3DFactory;
extern ID3D12Debug1* gDebugController;
extern IDXGIAdapter1* gD3DAdapter;
extern ID3D12Device* gD3DDevice;
extern ID3D12DebugDevice* gD3DDebugDevice;
extern ID3D12CommandQueue* gD3DCommandQueue;
extern ID3D12CommandAllocator* gD3DCommandAllocator;
extern ID3D12GraphicsCommandList* gCurrentCommandBuffer;
extern ID3D12DescriptorHeap* gDescriptorsHeapRTV;
extern ID3D12DescriptorHeap* gDescriptorsHeapDSV;
extern ID3D12DescriptorHeap* gDescriptorsHeapSRV;
extern UINT gCurrentSRVOffset;

extern std::vector<CD3DX12_SHADER_BYTECODE> gAllShaders;

struct PSOInfo
{
	ID3D12PipelineState* pso = nullptr;
	ID3D12RootSignature* rootSignature = nullptr;
	D3D_PRIMITIVE_TOPOLOGY primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mercury::i8 rootParameterRootConstantIndex = -1;
	mercury::i8 setOffsets[4] = { 0,0,0,0 };
};

extern std::vector<PSOInfo> gAllPSOs;

extern std::vector<ID3D12Resource*> gAllBuffers;
extern std::vector<BufferInfo> gAllBuffersMeta;

extern D3D12MA::Allocator* gAllocator;
extern DXGI_FORMAT gD3DSwapChainFormat;

extern std::vector<mercury::ll::graphics::ParameterBlockDescriptor> gAllParameterBlocks;

extern std::vector<TextureInfo> gAllTextures;

struct FrameData
{
	HANDLE fenceEvent = nullptr;
	ID3D12Fence* fence = nullptr;
	UINT64 fenceValue = 0;
	ID3D12GraphicsCommandList* commandList = nullptr;
	ID3D12CommandAllocator* commandAllocator = nullptr;
	mercury::u64 frameIndex = 0;
};

extern std::vector<FrameData> gFrames;

extern UINT gMSAASampleCount;
extern UINT gMSAAQuality;

extern mercury::u32 gFrameRingCurrent;

#define D3D_CALL(func) {HRESULT res = (func); if(res < 0){ mercury::ll::os::gOS->FatalFail( std::system_category().message(res).c_str() ); } }

#endif