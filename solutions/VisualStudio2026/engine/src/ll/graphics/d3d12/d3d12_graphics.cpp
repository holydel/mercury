void CommandList::PushConstants(const void* data, size_t size)
{
    auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);
    // Root parameter 0 is for push constants
    cmdListD3D12->SetGraphicsRoot32BitConstants(0, size / 4, data, 0);
}

void CommandList::BindSceneConstants(BufferHandle bufferHandle)
{
    auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(nativePtr);
    auto bufferResource = gAllBuffers[bufferHandle.handle];
    
    // Get the GPU virtual address of the buffer
  D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = bufferResource->GetGPUVirtualAddress();
    
    // Bind to root parameter 2 which corresponds to b0, space1 (set 1, binding 0)
    // Root parameter layout:
    // - 0: Push constants (if any)
  // - 1: CBV b1, space0 (per-object data) 
    // - 2: CBV b0, space1 (per-frame/scene data) <-- THIS ONE
    cmdListD3D12->SetGraphicsRootConstantBufferView(2, cbvAddress);
}
