#pragma once

#include "ll/graphics.h"

#ifdef MERCURY_LL_GRAPHICS_METAL

// Forward declarations for missing types
enum class BufferUsage {
    Vertex,
    Index,
    Uniform,
    Storage
};

enum class TextureUsage {
    ShaderRead,
    ShaderWrite,
    RenderTarget,
    DepthStencil
};

struct PipelineStateDesc {
    // Add pipeline state description fields as needed
    // This is a placeholder for now
};

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>

namespace mercury::ll::graphics {

// Metal-specific global objects
extern id<MTLDevice> gMetalNativeDevice;
extern id<MTLCommandQueue> gMetalCommandQueue;
extern id<MTLCommandBuffer> gMetalCommandBuffer;
extern id<MTLRenderCommandEncoder> gMetalRenderEncoder;
extern CAMetalLayer* gMetalLayer;
extern id<CAMetalDrawable> gMetalCurrentDrawable;

// Metal format conversion utilities
MTLPixelFormat ConvertFormatToMetal(Format format);
Format ConvertFormatFromMetal(MTLPixelFormat format);

// Metal utility functions
mercury::u64 GetMetalDeviceMemorySize(id<MTLDevice> device);
mercury::u32 GetMetalVendorId(id<MTLDevice> device);
mercury::u32 GetMetalDeviceId(id<MTLDevice> device);
const char* GetMetalDeviceName(id<MTLDevice> device);

// Metal graphics structures (no inheritance, no virtual calls)
struct MetalInstance {
    NSArray<id<MTLDevice>>* availableDevices;
    bool initialized;
    
    void Initialize();
    void Shutdown();
    void AcquireAdapter(const AdapterSelectorInfo& info);
};

struct MetalAdapter {
    id<MTLDevice> device;
    NSString* deviceName;
    mercury::u64 memorySize;
    mercury::u32 vendorId;
    mercury::u32 deviceId;
    bool initialized;
    
    void Initialize();
    void Shutdown();
    void CreateDevice();
    const char* GetName() const;
    mercury::u64 GetMemorySize() const;
    mercury::u32 GetVendorId() const;
    mercury::u32 GetDeviceId() const;
    void SetDevice(id<MTLDevice> device);
};

struct MetalDevice {
    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;
    id<MTLCommandBuffer> currentCommandBuffer;
    id<MTLRenderCommandEncoder> currentRenderEncoder;
    bool initialized;
    
    void Initialize();
    void Shutdown();
    void InitializeSwapchain();
    void BeginFrame();
    void EndFrame();
    void Present();
    void* GetNativeDevice() const;
    void SetDevice(id<MTLDevice> device);
};

struct MetalSwapchain {
    CAMetalLayer* metalLayer;
    id<CAMetalDrawable> currentDrawable;
    mercury::u32 width;
    mercury::u32 height;
    Format format;
    bool initialized;
    
    void Initialize(mercury::u32 width, mercury::u32 height, Format format);
    void Shutdown();
    void Resize(mercury::u32 width, mercury::u32 height);
    mercury::u32 GetWidth() const;
    mercury::u32 GetHeight() const;
    Format GetFormat() const;
    void* GetNativeSwapchain() const;
};

// Metal shader compilation
struct MetalShaderCompiler {
    static ShaderHandle CompileShader(const char* source, ShaderStage stage);
    static void DestroyShader(ShaderHandle handle);
    
private:
    static id<MTLLibrary> CompileLibrary(const char* source);
    static MTLFunctionType GetFunctionType(ShaderStage stage);
};

// Metal pipeline state object
struct MetalPipelineState {
    id<MTLRenderPipelineState> pso;
    MTLRenderPipelineDescriptor* descriptor;
    bool created;
    
    void Create(const PipelineStateDesc& desc);
    void Destroy();
    id<MTLRenderPipelineState> GetNativePSO() const;
};

// Metal buffer management
struct MetalBuffer {
    id<MTLBuffer> buffer;
    mercury::u64 size;
    BufferUsage usage;
    bool created;
    
    void Create(mercury::u64 size, BufferUsage usage);
    void Destroy();
    void* Map();
    void Unmap();
    id<MTLBuffer> GetNativeBuffer() const;
    mercury::u64 GetSize() const;
};

// Metal texture management
struct MetalTexture {
    id<MTLTexture> texture;
    mercury::u32 width;
    mercury::u32 height;
    Format format;
    TextureUsage usage;
    bool created;
    
    void Create(mercury::u32 width, mercury::u32 height, Format format, TextureUsage usage);
    void Destroy();
    id<MTLTexture> GetNativeTexture() const;
    mercury::u32 GetWidth() const;
    mercury::u32 GetHeight() const;
    Format GetFormat() const;
};

// Metal command buffer management
struct MetalCommandBuffer {
    id<MTLCommandBuffer> commandBuffer;
    id<MTLRenderCommandEncoder> renderEncoder;
    bool isRecording;
    
    void Begin();
    void End();
    void Submit();
    id<MTLCommandBuffer> GetNativeCommandBuffer() const;
    id<MTLRenderCommandEncoder> GetRenderEncoder() const;
};

// Global Metal graphics objects (declared in cpp file)

// Metal graphics API functions (no virtual calls)
const char* GetMetalBackendName();
void MetalGraphicsInitialize();
void MetalGraphicsShutdown();
void MetalGraphicsBeginFrame();
void MetalGraphicsEndFrame();
void MetalGraphicsPresent();

} // namespace mercury::ll::graphics

#endif // MERCURY_LL_GRAPHICS_METAL