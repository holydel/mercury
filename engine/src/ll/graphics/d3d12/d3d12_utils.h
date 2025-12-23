#pragma once
#include "ll/graphics.h"

#if defined(MERCURY_LL_GRAPHICS_D3D12)
#include <dxgi1_4.h>
#include "directx/d3dx12.h"

mercury::ll::graphics::Format FromDXGIFormat(DXGI_FORMAT dxgiFormat);
DXGI_FORMAT ToDXGIFormat(mercury::ll::graphics::Format format);

void PrintShaderBytecodeInputs(CD3DX12_SHADER_BYTECODE& bytecode);

D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyTypeFromMercuryTopology(mercury::ll::graphics::PrimitiveTopology topology);
D3D_PRIMITIVE_TOPOLOGY PrimitiveTopologyFromMercuryPrimitiveTopology(mercury::ll::graphics::PrimitiveTopology topology);

// Helper functions for fence synchronization
mercury::u64 Signal(ID3D12CommandQueue* commandQueue, ID3D12Fence* fence, mercury::u64& fenceValue);
void WaitForFenceValue(ID3D12Fence* fence, mercury::u64 fenceValue, HANDLE fenceEvent);

void Flush(ID3D12CommandQueue* commandQueue, ID3D12Fence* fence, mercury::u64& fenceValue, HANDLE fenceEvent);
#endif