struct CommandList
{
  void* nativePtr = nullptr;  
  void* currentRenderPassNativePtr = nullptr;
  void* currentPSOnativePtr = nullptr;
  void* currentPSOLayoutNativePtr = nullptr;

  bool IsExecuted();
  void SetDebugName(const char* utf8_name);
  void Destroy();

  void RenderImgui();

  void SetPSO(Handle<u32> psoID);
  void Draw(u32 vertexCount, u32 instanceCount = 1, u32 firstVertex = 0, u32 firstInstance = 0);

  void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);
  void SetScissor(i32 x, i32 y, u32 width, u32 height);

  void PushConstants(const void* data, size_t size);  

  template <typename T>
  void PushConstants(const T& data)
  {
  PushConstants(&data, sizeof(T));
  }

  /// @brief Bind a constant buffer to set 1 (per-frame/scene data)
  /// This should be called once per frame before rendering materials
  void BindSceneConstants(BufferHandle bufferHandle);
};
