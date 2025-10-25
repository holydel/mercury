        IF_LIKELY(gSwapchain)
    {
            // Swapchain available, acquiring next image
     auto finalCmdList = gSwapchain->AcquireNextImage();
			u32 currentFrameIndex = gSwapchain->GetCurrentFrameIndex();

            static float ctime = 0.0f;
			ctime += 0.016f; // TODO: Replace with actual delta time

			Scene2DConstants scene2DConstants = {};
			scene2DConstants.canvasSize = glm::vec4((float)gSwapchain->GetWidth(), (float)gSwapchain->GetHeight(), 1.0f / (float)gSwapchain->GetWidth(), 1.0f / (float)gSwapchain->GetHeight());
         scene2DConstants.time = ctime;
			scene2DConstants.deltaTime = 0.016f; // TODO: Replace with actual delta time
			scene2DConstants.prerptationMatrix = glm::mat2x2(1.0f); // Identity matrix for now

			gDevice->UpdateBuffer(gPerFrameResources[currentFrameIndex].scene2DConstantBuffer, &scene2DConstants, sizeof(Scene2DConstants));

			finalCmdList.SetViewport(0, 0, (float)gSwapchain->GetWidth(), (float)gSwapchain->GetHeight());
			finalCmdList.SetScissor(0, 0, (u32)gSwapchain->GetWidth(), (u32)gSwapchain->GetHeight());

#if defined(MERCURY_LL_GRAPHICS_D3D12)
			// Bind scene constants once for all materials (D3D12 specific)
			// This binds the constant buffer to root parameter 2 (b0, space1 = set 1, binding 0)
			extern std::vector<ID3D12Resource*> gAllBuffers;
			auto cmdListD3D12 = static_cast<ID3D12GraphicsCommandList*>(finalCmdList.nativePtr);
			auto bufferResource = gAllBuffers[gPerFrameResources[currentFrameIndex].scene2DConstantBuffer.handle];
			D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = bufferResource->GetGPUVirtualAddress();
			cmdListD3D12->SetGraphicsRootConstantBufferView(2, cbvAddress);
#endif

            mercury::Application::GetCurrentApplication()->OnFinalPass(finalCmdList);

      mercury_imgui::BeginFrame(finalCmdList);         
          mercury::Application::GetCurrentApplication()->OnImgui();
  mercury_imgui::EndFrame(finalCmdList);
   // do all graphics job here
       gSwapchain->Present();
   }
