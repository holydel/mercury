#include "d3d12_command_list.h"

#ifdef MERCURY_LL_GRAPHICS_D3D12

using namespace mercury;
using namespace mercury::ll::graphics;

#include "../../../imgui/imgui_impl.h"
#include <mercury_log.h>

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

void CommandList::RenderImgui()
{
	auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);
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

void CommandList::SetIndexBuffer(BufferHandle bufferID)
{
	auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);

	D3D12_INDEX_BUFFER_VIEW view = {};
	const auto& bmeta = gAllBuffersMeta[bufferID.handle];
	view.Format = DXGI_FORMAT_R16_UINT;
	view.SizeInBytes = static_cast<UINT>(bmeta.size);
	view.BufferLocation = bmeta.gpuAddress;

	cmdListD3D12->IASetIndexBuffer(&view);
}

void CommandList::SetVertexBuffer(BufferHandle bufferID, u8 stride, u8 slot, size_t offset)
{
	auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);

	D3D12_VERTEX_BUFFER_VIEW view = {};
	const auto& bmeta = gAllBuffersMeta[bufferID.handle];

	view.StrideInBytes = stride;
	view.SizeInBytes = static_cast<UINT>(bmeta.size);
	view.BufferLocation = bmeta.gpuAddress + offset;

	cmdListD3D12->IASetVertexBuffers(slot, 1, &view);
}

void CommandList::DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, u32 firstVertex, u32 firstInstance)
{
	auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);
	cmdListD3D12->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, firstVertex, firstInstance);
}

void CommandList::PushConstants(const void* data, size_t size)
{
	auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);

	cmdListD3D12->SetGraphicsRoot32BitConstants(gAllPSOs[currentPsoID.handle].rootParameterRootConstantIndex, static_cast<UINT>(size / 4), data, 0);
}

void  CommandList::SetParameterBlockLayout(u8 set_index, ParameterBlockLayoutHandle layoutID)
{
	auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);
	cmdListD3D12->SetGraphicsRootSignature(gAllPSOs[layoutID.handle].rootSignature);
}


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
					// Intentionally empty slot skip writing
				}
			}, res);

		slotIndex++;
	}
}
#endif