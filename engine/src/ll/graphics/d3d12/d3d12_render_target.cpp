#include "d3d12_render_target.h"

#ifdef MERCURY_LL_GRAPHICS_D3D12
#include <mercury_log.h>
#include "d3d12_utils.h"

using namespace mercury;
using namespace mercury::ll::graphics;

std::vector<RenderTargetInfo> gAllRenderTargets;

CD3DX12_CPU_DESCRIPTOR_HANDLE gRTVHeapStartCPU;
CD3DX12_CPU_DESCRIPTOR_HANDLE gDSVHeapStartCPU;

UINT gRTVDescriptorSize = 0;
UINT gDSVDescriptorSize = 0;

constexpr int MAX_RENDER_TARGETS = 512;

std::array<bool, MAX_RENDER_TARGETS> gSlotsUsedRTV = {};
std::array<bool, MAX_RENDER_TARGETS> gSlotsUsedDSV = {};

CD3DX12_CPU_DESCRIPTOR_HANDLE GetRTVDescriptorHandle()
{
	for (int i = 0; i < MAX_RENDER_TARGETS; ++i)
	{
		if (!gSlotsUsedRTV[i])
		{
			gSlotsUsedRTV[i] = true;
			return CD3DX12_CPU_DESCRIPTOR_HANDLE(gRTVHeapStartCPU, i, gRTVDescriptorSize);
		}
	}
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(gRTVHeapStartCPU, 0, gRTVDescriptorSize);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE GetDSVDescriptorHandle()
{
	for (int i = 0; i < MAX_RENDER_TARGETS; ++i)
	{
		if (!gSlotsUsedDSV[i])
		{
			gSlotsUsedDSV[i] = true;
			return CD3DX12_CPU_DESCRIPTOR_HANDLE(gDSVHeapStartCPU, i, gDSVDescriptorSize);
		}
	}
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(gDSVHeapStartCPU, 0, gDSVDescriptorSize);
}

void FreeRTVDescriptorHandle(const CD3DX12_CPU_DESCRIPTOR_HANDLE& handle)
{
	int index = (handle.ptr - gRTVHeapStartCPU.ptr) / gRTVDescriptorSize;
	if (index >= 0 && index < MAX_RENDER_TARGETS)
	{
		gSlotsUsedRTV[index] = false;
	}
}

void FreeDSVDescriptorHandle(const CD3DX12_CPU_DESCRIPTOR_HANDLE& handle)
{
	int index = (handle.ptr - gDSVHeapStartCPU.ptr) / gDSVDescriptorSize;
	if (index >= 0 && index < MAX_RENDER_TARGETS)
	{
		gSlotsUsedDSV[index] = false;
	}
}

void InitRenderTargetHeaps()
{

	gRTVDescriptorSize = gD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	gDSVDescriptorSize = gD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = MAX_RENDER_TARGETS;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		D3D_CALL(gD3DDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gDescriptorsHeapRTV)));
		gDescriptorsHeapRTV->SetName(L"RTV Heap");

		gRTVHeapStartCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(gDescriptorsHeapRTV->GetCPUDescriptorHandleForHeapStart());
		
	}

	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = MAX_RENDER_TARGETS;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		D3D_CALL(gD3DDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&gDescriptorsHeapDSV)));
		gDescriptorsHeapDSV->SetName(L"DSV Heap");

		gDSVHeapStartCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(gDescriptorsHeapDSV->GetCPUDescriptorHandleForHeapStart());
	}
}

RenderTargetHandle CreateRenderTarget(const RenderTargetCreateDescriptor& desc)
{
	auto& rtt =  gAllRenderTargets.emplace_back();

	rtt.useStaticClear = (desc.staticClear != nullptr);
	if (desc.staticClear)
	{
		rtt.staticClearInfo = *desc.staticClear;
	}

	rtt.targetInfo = static_cast<TargetInfo>(desc);

	int numColorTargets = 0;

	for (int i = 0; i < 8; ++i)
	{
		if (desc.colorFormat[i] != Format::NONE)
		{
			rtt.colorFormat[numColorTargets++] = ToDXGIFormat(desc.colorFormat[i]);
			++numColorTargets;
		}
	}

	// Check MSAA quality support
	if (desc.numSamples > 1 && numColorTargets > 0)
	{
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels = {};
		msaaQualityLevels.Format = rtt.colorFormat[0];
		msaaQualityLevels.SampleCount = desc.numSamples;
		msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;

		//gD3DDevice->CheckFeatureSupport(
		//	D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		//	&msaaQualityLevels,
		//	sizeof(msaaQualityLevels)
		//);

		//if (msaaQualityLevels.NumQualityLevels > 0)
		//{
		//	rtt.MSAAQuality = msaaQualityLevels.NumQualityLevels - 1;
		//	MLOG_DEBUG(u8"MSAA %ux supported with %u quality levels", gMSAASampleCount, msaaQualityLevels.NumQualityLevels);
		//}
		//else
		//{
		//	MLOG_WARNING(u8"MSAA %ux not supported, falling back to no MSAA", gMSAASampleCount);
		//	rtt.MSAASampleCount = 1;
		//	rtt.MSAAQuality = 0;
		//}

		// Create MSAA render target
		if (rtt.targetInfo.numSamples > 1)
		{
			D3D12_RESOURCE_DESC msaaRTDesc = {};
			msaaRTDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			msaaRTDesc.Alignment = 0;
			msaaRTDesc.Width = desc.width;
			msaaRTDesc.Height = desc.height;
			msaaRTDesc.DepthOrArraySize = 1;
			msaaRTDesc.MipLevels = 1;
			msaaRTDesc.Format = rtt.colorFormat[0];
			msaaRTDesc.SampleDesc.Count = rtt.targetInfo.numSamples;
			msaaRTDesc.SampleDesc.Quality = 0;
			msaaRTDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			msaaRTDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

			D3D12_CLEAR_VALUE msaaClearValue = {};
			msaaClearValue.Format = msaaRTDesc.Format;
			msaaClearValue.Color[0] = 0.0f;
			msaaClearValue.Color[1] = 0.0f;
			msaaClearValue.Color[2] = 0.0f;
			msaaClearValue.Color[3] = 1.0f;

			D3D12MA::ALLOCATION_DESC msaaAllocDesc = {};
			msaaAllocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
			msaaAllocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

			HRESULT hr = gAllocator->CreateResource(
				&msaaAllocDesc,
				&msaaRTDesc,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				&msaaClearValue,
				&rtt.MSAARenderTargetAllocation,
				IID_PPV_ARGS(&rtt.MSAARenderTarget)
			);

			rtt.MSAARenderTargetView = GetRTVDescriptorHandle();

			D3D12_RENDER_TARGET_VIEW_DESC msaaRtvDesc = {};
			msaaRtvDesc.Format = msaaRTDesc.Format;
			msaaRtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

			gD3DDevice->CreateRenderTargetView(rtt.MSAARenderTarget, &msaaRtvDesc, rtt.MSAARenderTargetView);
			MLOG_DEBUG(u8"Created MSAA render target: %ux%u %ux MSAA", rtt.targetInfo.width, rtt.targetInfo.height, rtt.targetInfo.numSamples);

		}
	}
	
	if (desc.depthStencilFormat != Format::NONE)
	{
		// Create depth-stencil buffer
		D3D12_RESOURCE_DESC depthDesc = {};
		depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthDesc.Alignment = 0;
		depthDesc.Width = rtt.targetInfo.width;
		depthDesc.Height = rtt.targetInfo.height;
		depthDesc.DepthOrArraySize = 1;
		depthDesc.MipLevels = 1;
		depthDesc.Format = ToDXGIFormat(desc.depthStencilFormat);
		depthDesc.SampleDesc.Count = desc.numSamples;
		depthDesc.SampleDesc.Quality = 0;
		depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE depthClearValue = {};
		depthClearValue.Format = depthDesc.Format;
		depthClearValue.DepthStencil.Depth = 1.0f;
		depthClearValue.DepthStencil.Stencil = 0;

		D3D12MA::ALLOCATION_DESC depthAllocDesc = {};
		depthAllocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		//depthAllocDesc.Flags = D3D12MA::ALLOCATOR_FLAG_COMMITTED;

		HRESULT hr = gAllocator->CreateResource(
			&depthAllocDesc,
			&depthDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthClearValue,
			&rtt.DepthStencilAllocation,
			IID_PPV_ARGS(&rtt.DepthStencilBuffer)
		);

		// Create depth-stencil view
		rtt.DepthStencilView = GetDSVDescriptorHandle();

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = depthDesc.Format;
		dsvDesc.ViewDimension = rtt.targetInfo.numSamples > 1 ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		gD3DDevice->CreateDepthStencilView(rtt.DepthStencilBuffer, &dsvDesc, rtt.DepthStencilView);

		MLOG_DEBUG(u8"Created depth-stencil buffer: %ux%u", rtt.targetInfo.width, rtt.targetInfo.height);

	}
	

	return RenderTargetHandle{ (u16)(gAllRenderTargets.size() - 1)};
}

TargetInfo RenderTargetHandle::GetTargetInfo()
{
	return gAllRenderTargets[this->handle].targetInfo;
}

void RenderTargetHandle::BeginPass()
{

}

void RenderTargetHandle::EndPass()
{

}

void RenderTargetHandle::SetDebugName(const char* utf8_name)
{

}

void RenderTargetHandle::SetFramebuffers(void** nativeFramebuffers, u8 numBuffers)
{
	auto& rtt = gAllRenderTargets[this->handle];
	
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = gD3DSwapChainFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	gD3DDevice->CreateRenderTargetView((ID3D12Resource*)nativeFramebuffers[i], &rtvDesc, rtvHandle);
	bb.bbRTV = rtvHandle;

	rtvHandle.Offset(rtvDescriptorSize);
}

void RenderTargetHandle::SetFramebufferIndex(u32 imageIndex)
{
	gAllRenderTargets[this->handle].currentFramebufferIndex = imageIndex;
}

void RenderTargetHandle::ResizeIfNeeded(u16 newWidth, u16 newHeight)
{

}

#endif