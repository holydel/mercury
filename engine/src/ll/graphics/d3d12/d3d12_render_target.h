#pragma once
#include "d3d12_graphics.h"

#ifdef MERCURY_LL_GRAPHICS_D3D12

struct RenderTargetFramebufferInfo
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ID3D12Resource* resource = nullptr;
};

struct RenderTargetExternalFramebufferInfo
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE externalRTV;
	ID3D12Resource* externalResource = nullptr;
};

struct RenderTargetInfo
{
	mercury::ll::graphics::RenderTargetClearInfo staticClearInfo;
	mercury::ll::graphics::TargetInfo targetInfo;

	DXGI_FORMAT colorFormat[8] = { DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN
		,DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN };

	DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_UNKNOWN;

	bool useStaticClear = false;
	u32 currentFramebufferIndex = 0;
	u32 numFramebuffers = 0;

	std::vector<RenderTargetFramebufferInfo> colorFramebuffers;

	CD3DX12_CPU_DESCRIPTOR_HANDLE internalRTV;
	ID3D12Resource* internalResource = nullptr;

	// MSAA render target resources
	ID3D12Resource* MSAARenderTarget = { nullptr };
	D3D12MA::Allocation* MSAARenderTargetAllocation = { nullptr };
	CD3DX12_CPU_DESCRIPTOR_HANDLE MSAARenderTargetView = {};

	// Depth buffer resources
	ID3D12Resource* DepthStencilBuffer = nullptr;
	D3D12MA::Allocation* DepthStencilAllocation = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE DepthStencilView = {};
};

extern std::vector<RenderTargetInfo> gAllRenderTargets;

void InitRenderTargetHeaps();
#endif