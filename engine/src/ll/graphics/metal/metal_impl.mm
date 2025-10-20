#include "ll/graphics.h"
#include "mercury_log.h"
#include "mercury_application.h"
#include "ll/os.h"
#include "mercury_api.h"
#include "graphics.h"

#ifdef MERCURY_LL_GRAPHICS_METAL

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/CAMetalLayer.h>
#include "../../../imgui/imgui_impl.h"

using namespace mercury::ll::graphics;

// Metal global objects
static id<MTLDevice> gMetalDevice = nil;
static id<MTLCommandQueue> gMetalCommandQueue = nil;
static id<MTLCommandBuffer> gMetalCommandBuffer = nil;
static id<MTLRenderCommandEncoder> gMetalRenderEncoder = nil;
static id<MTLRenderCommandEncoder> gMetalRenderPassEncoder = nil;
static MTLRenderPassColorAttachmentDescriptor *gMetalRenderPassColorAttachment = nil;
static MTLRenderPassDescriptor *gMetalRenderPassDescriptor = nil;
static CAMetalLayer* gMetalLayer = nil;
static id<CAMetalDrawable> gMetalCurrentDrawable = nil;

// Metal format conversion utilities
static MTLPixelFormat ConvertFormatToMetal(Format format) {
    switch (format) {
        case Format::RGBA8_UNORM:        return MTLPixelFormatRGBA8Unorm;
        case Format::RGBA8_UNORM_SRGB:   return MTLPixelFormatRGBA8Unorm_sRGB;
        case Format::BGRA8_UNORM:        return MTLPixelFormatBGRA8Unorm;
        case Format::RGBA16_FLOAT:       return MTLPixelFormatRGBA16Float;
        case Format::RGBA32_FLOAT:       return MTLPixelFormatRGBA32Float;
        case Format::R8_UNORM:           return MTLPixelFormatR8Unorm;
        case Format::RG8_UNORM:          return MTLPixelFormatRG8Unorm;
        case Format::R16_FLOAT:          return MTLPixelFormatR16Float;
        case Format::RG16_FLOAT:         return MTLPixelFormatRG16Float;
        case Format::R32_FLOAT:          return MTLPixelFormatR32Float;
        case Format::RG32_FLOAT:         return MTLPixelFormatRG32Float;
        case Format::DEPTH24_UNORM_STENCIL8: return MTLPixelFormatDepth24Unorm_Stencil8;
        case Format::DEPTH32_FLOAT:      return MTLPixelFormatDepth32Float;
        case Format::DEPTH16_UNORM:      return MTLPixelFormatDepth16Unorm;
        case Format::R8_SNORM:           return MTLPixelFormatR8Snorm;
        case Format::R8_UINT:            return MTLPixelFormatR8Uint;
        case Format::R8_SINT:            return MTLPixelFormatR8Sint;
        case Format::RG8_SNORM:          return MTLPixelFormatRG8Snorm;
        case Format::RG8_UINT:           return MTLPixelFormatRG8Uint;
        case Format::RG8_SINT:           return MTLPixelFormatRG8Sint;
        case Format::RGBA8_SNORM:        return MTLPixelFormatRGBA8Snorm;
        case Format::RGBA8_UINT:         return MTLPixelFormatRGBA8Uint;
        case Format::RGBA8_SINT:         return MTLPixelFormatRGBA8Sint;
        case Format::R16_UNORM:          return MTLPixelFormatR16Unorm;
        case Format::R16_SNORM:          return MTLPixelFormatR16Snorm;
        case Format::R16_UINT:           return MTLPixelFormatR16Uint;
        case Format::R16_SINT:           return MTLPixelFormatR16Sint;
        case Format::RG16_UNORM:         return MTLPixelFormatRG16Unorm;
        case Format::RG16_SNORM:         return MTLPixelFormatRG16Snorm;
        case Format::RG16_UINT:          return MTLPixelFormatRG16Uint;
        case Format::RG16_SINT:          return MTLPixelFormatRG16Sint;
        case Format::RGBA16_UNORM:       return MTLPixelFormatRGBA16Unorm;
        case Format::RGBA16_SNORM:       return MTLPixelFormatRGBA16Snorm;
        case Format::RGBA16_UINT:        return MTLPixelFormatRGBA16Uint;
        case Format::RGBA16_SINT:        return MTLPixelFormatRGBA16Sint;
        case Format::R32_UINT:           return MTLPixelFormatR32Uint;
        case Format::R32_SINT:           return MTLPixelFormatR32Sint;
        case Format::RG32_UINT:          return MTLPixelFormatRG32Uint;
        case Format::RG32_SINT:          return MTLPixelFormatRG32Sint;
        case Format::RGBA32_UINT:        return MTLPixelFormatRGBA32Uint;
        case Format::RGBA32_SINT:        return MTLPixelFormatRGBA32Sint;
        case Format::R64_FLOAT:          return MTLPixelFormatR32Float; // Fallback
        case Format::RG64_FLOAT:         return MTLPixelFormatRG32Float; // Fallback
        case Format::RGBA64_FLOAT:       return MTLPixelFormatRGBA32Float; // Fallback
        case Format::R10G10B10A2_UNORM:  return MTLPixelFormatRGB10A2Unorm;
        case Format::R10G10B10A2_UINT:   return MTLPixelFormatRGB10A2Uint;
        case Format::RG11B10_FLOAT:      return MTLPixelFormatRG11B10Float;
        case Format::R9G9B9E5_UFLOAT:    return MTLPixelFormatRGB9E5Float;
        case Format::B5G6R5_UNORM:       return MTLPixelFormatB5G6R5Unorm;
        case Format::B5G5R5A1_UNORM:     return MTLPixelFormatBGR5A1Unorm;
        case Format::A1R5G5B5_UNORM:     return MTLPixelFormatABGR4Unorm;
        case Format::STENCIL8_UINT:      return MTLPixelFormatStencil8;
        case Format::DEPTH32_FLOAT_STENCIL8: return MTLPixelFormatDepth32Float_Stencil8;
        case Format::DEPTH16_UNORM_STENCIL8: return MTLPixelFormatDepth24Unorm_Stencil8; // Fallback
        default:
            MLOG_WARNING(u8"Unsupported format conversion to Metal: %d", static_cast<int>(format));
            return MTLPixelFormatRGBA8Unorm;
    }
}

// Metal utility functions
static mercury::u64 GetMetalDeviceMemorySize(id<MTLDevice> device) {
    if (@available(macOS 10.15, iOS 13.0, *)) {
        return device.recommendedMaxWorkingSetSize;
    }
    return 0; // Fallback for older versions
}

static mercury::u32 GetMetalVendorId(id<MTLDevice> device) {
    NSString* deviceName = device.name;
    if ([deviceName containsString:@"Apple"]) {
        return 0x106B; // Apple vendor ID
    }
    return 0; // Unknown vendor
}

static mercury::u32 GetMetalDeviceId(id<MTLDevice> device) {
    return 0; // Not available in Metal API
}

// Metal adapter enumeration
static AdapterInfo GetInfoFromMetalDevice(id<MTLDevice> device) {
    AdapterInfo info;
    
    // Get device name
    NSString* deviceName = device.name;
    const char* nameCStr = [deviceName UTF8String];
    info.name = std::u8string(reinterpret_cast<const char8_t*>(nameCStr));
    
    // Set device type (Metal doesn't distinguish between integrated/discrete in the same way)
    // We'll assume integrated for now, but this could be enhanced with more sophisticated detection
    info.type = AdapterInfo::Type::Integrated;
    
    // Set vendor (Apple for Metal devices)
    info.vendor = AdapterInfo::Vendor::Apple;
    
    // Get vendor and device IDs
    info.vendor_id = GetMetalVendorId(device);
    info.device_id = GetMetalDeviceId(device);
    
    // Set driver version (Metal doesn't expose this directly)
    info.driver_version = u8"Metal";
    
    // Set vendor name
    info.vendor_name = u8"Apple Inc.";
    
    return info;
}

void EnumerateAllMetalDevices() {
    NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
    if (!devices || devices.count == 0) {
        MLOG_WARNING(u8"No Metal devices found");
        return;
    }
    
    MLOG_INFO(u8"Found %d Metal devices", (int)devices.count);
    
    gAllAdaptersInfo.resize(devices.count);
    
    for (NSUInteger i = 0; i < devices.count; ++i) {
        id<MTLDevice> device = devices[i];
        gAllAdaptersInfo[i] = GetInfoFromMetalDevice(device);
        
        MLOG_DEBUG(u8"Metal device %d: %s", (int)i, gAllAdaptersInfo[i].name.c_str());
    }
}

// Instance implementation
void Instance::Initialize() {
    MLOG_DEBUG(u8"Initializing Metal Instance");
    
    // Enumerate all available Metal devices
    EnumerateAllMetalDevices();
}

void Instance::Shutdown() {
    MLOG_DEBUG(u8"Shutting down Metal Instance");
    // Metal doesn't need explicit instance shutdown
}

void* Instance::GetNativeHandle() {
    return nullptr; // Metal doesn't expose instance handle
}


void Instance::AcquireAdapter(const AdapterSelectorInfo& info) {
    MLOG_DEBUG(u8"Acquiring Metal Adapter");
    
    NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
    if (!devices || devices.count == 0) {
        MLOG_ERROR(u8"No Metal devices available");
        return;
    }
    
    id<MTLDevice> selectedDevice = nil;
    
    if (info.adapter_index < devices.count) {
        selectedDevice = devices[info.adapter_index];
    } else {
        // Select default device (usually the integrated GPU)
        selectedDevice = devices[0];
    }
    
    if (!selectedDevice) {
        MLOG_ERROR(u8"Failed to select Metal device");
        return;
    }
    
    MLOG_DEBUG(u8"Selected Metal device: %s", [selectedDevice.name UTF8String]);
    
    // Set the global device
    gMetalDevice = selectedDevice;

    if(!gAdapter)
        gAdapter = new Adapter();
}

// Adapter implementation
void Adapter::Initialize() {
    MLOG_DEBUG(u8"Initializing Metal Adapter");
    
    if (!gMetalDevice) {
        MLOG_ERROR(u8"No Metal device set for adapter");
        return;
    }
    
    MLOG_DEBUG(u8"Adapter: %s", [gMetalDevice.name UTF8String]);
    MLOG_DEBUG(u8"Memory: %llu MB", GetMetalDeviceMemorySize(gMetalDevice) / (1024 * 1024));
}

void Adapter::Shutdown() {
    MLOG_DEBUG(u8"Shutting down Metal Adapter");
}

void* Adapter::GetNativeHandle() {
    return (__bridge void*)gMetalDevice;
}

void Adapter::CreateDevice() {
    MLOG_DEBUG(u8"Creating Metal Device");
    
    if (!gMetalDevice) {
        MLOG_ERROR(u8"No Metal device available for device creation");
        return;
    }
    
    // Create command queue
    gMetalCommandQueue = [gMetalDevice newCommandQueue];
    
    if (!gMetalCommandQueue) {
        MLOG_ERROR(u8"Failed to create Metal command queue");
        return;
    }
    
    if(!gDevice)
        gDevice = new Device();

    MLOG_DEBUG(u8"Metal Device created successfully");

    // Ensure any existing CAMetalLayer has its device set early so that
    // nextDrawable() won't return nil if the layer was created before the
    // device. This avoids races where the view/layer exists but device is
    // assigned later during swapchain init.
    if (gMetalLayer && [gMetalLayer respondsToSelector:@selector(setDevice:)]) {
        gMetalLayer.device = gMetalDevice;
        MLOG_DEBUG(u8"Early-assigned gMetalDevice to CAMetalLayer.device in Adapter::CreateDevice: %s", [gMetalDevice.name UTF8String]);
    }
}

// Device implementation
void Device::Initialize() {
    MLOG_DEBUG(u8"Initializing Metal Device");
    
    if (!gMetalDevice) {
        MLOG_ERROR(u8"No Metal device set for device initialization");
        return;
    }
    
    // Create command queue
    gMetalCommandQueue = [gMetalDevice newCommandQueue];
    if (!gMetalCommandQueue) {
        MLOG_ERROR(u8"Failed to create Metal command queue");
        return;
    }
    
    MLOG_DEBUG(u8"Metal Device initialized successfully");
}

void Device::Shutdown() {
    MLOG_DEBUG(u8"Shutting down Metal Device");
    
    if (gMetalRenderEncoder) {
        [gMetalRenderEncoder endEncoding];
        gMetalRenderEncoder = nil;
    }
    
    if (gMetalCommandBuffer) {
        gMetalCommandBuffer = nil;
    }
    
    if (gMetalCommandQueue) {
        gMetalCommandQueue = nil;
    }
    
    gMetalDevice = nil;
}

void* Device::GetNativeHandle() {
    return (__bridge void*)gMetalDevice;
}

void Device::Tick() {
    // Metal device tick - nothing needed here
}

void Device::InitializeSwapchain() {
    MLOG_DEBUG(u8"Initializing Metal Swapchain");
    
    // Get the Metal layer from the macOS OS implementation
    void* nativeWindowHandle = ll::os::gOS->GetCurrentNativeWindowHandle();
    if (!nativeWindowHandle) {
        MLOG_ERROR(u8"No native window handle available for swapchain");
        return;
    }
    
    // The native window handle should be the NSView with the Metal layer
    NSView* view = (__bridge NSView*)nativeWindowHandle;
    if (!view || !view.layer || ![view.layer isKindOfClass:[CAMetalLayer class]]) {
        MLOG_ERROR(u8"Native window handle is not a view with Metal layer");
        return;
    }
    
    gMetalLayer = (CAMetalLayer*)view.layer;
    
    // Get window size
    unsigned int width, height;
    ll::os::gOS->GetActualWindowSize(width, height);
    
    // Configure the Metal layer
    gMetalLayer.pixelFormat = ConvertFormatToMetal(Format::BGRA8_UNORM);
    gMetalLayer.framebufferOnly = YES;
    gMetalLayer.drawableSize = CGSizeMake(width, height);

    // Ensure the CAMetalLayer has a Metal device assigned. If our selected
    // Metal device is available, use it. Otherwise try to fall back to the
    // system default device. A nil layer.device causes nextDrawable to return
    // nil on macOS (observed as "[CAMetalLayer nextDrawable] returning nil because device is nil").
    if ([gMetalLayer respondsToSelector:@selector(setDevice:)]) {
        if (gMetalDevice) {
            gMetalLayer.device = gMetalDevice;
            MLOG_DEBUG(u8"Assigned gMetalDevice to CAMetalLayer.device: %s", [gMetalDevice.name UTF8String]);
        } else {
            id<MTLDevice> sysDevice = MTLCreateSystemDefaultDevice();
            if (sysDevice) {
                gMetalLayer.device = sysDevice;
                // If we didn't already have a global device, adopt the system device
                if (!gMetalDevice) {
                    gMetalDevice = sysDevice;
                    MLOG_DEBUG(u8"No selected Metal device found; using system default device: %s", [sysDevice.name UTF8String]);
                } else {
                    MLOG_DEBUG(u8"No selected Metal device found; system default device available: %s", [sysDevice.name UTF8String]);
                }
            } else {
                MLOG_WARNING(u8"Failed to obtain system Metal device while initializing swapchain; CAMetalLayer.device remains nil");
            }
        }
    }
    
    MLOG_DEBUG(u8"Metal Swapchain initialized successfully");

    if(!gSwapchain)
        gSwapchain = new Swapchain();
}

void Device::ShutdownSwapchain() {
    MLOG_DEBUG(u8"Shutting down Metal Swapchain");
    
    if (gMetalCurrentDrawable) {
        gMetalCurrentDrawable = nil;
    }
    
    gMetalLayer = nil;
}

// IMGUI_IMPL_API bool ImGui_ImplMetal_Init(MTL::Device* device);
// IMGUI_IMPL_API void ImGui_ImplMetal_Shutdown();
// IMGUI_IMPL_API void ImGui_ImplMetal_NewFrame(MTL::RenderPassDescriptor* renderPassDescriptor);
// IMGUI_IMPL_API void ImGui_ImplMetal_RenderDrawData(ImDrawData* draw_data,
//                                                    MTL::CommandBuffer* commandBuffer,
//                                                    MTL::RenderCommandEncoder* commandEncoder);
void Device::ImguiInitialize() {
    MLOG_DEBUG(u8"Metal ImGui initialization placeholder");

    // Ensure Metal device and command queue are available before initializing ImGui Metal backend
    if (!gMetalDevice) {
        MLOG_WARNING(u8"Metal ImGui initialization skipped: no Metal device available yet");
        return;
    }
    if (!gMetalCommandQueue) {
        MLOG_WARNING(u8"Metal ImGui initialization skipped: no Metal command queue available yet");
        return;
    }

    // Safe to initialize ImGui Metal backend
    ImGui_ImplMetal_Init(gMetalDevice);
    // TODO: Implement Metal ImGui initialization
}

void Device::ImguiNewFrame() {
    // ImGui new frame (Metal)

    // Only call NewFrame if we have a valid render pass descriptor and command buffer
    if (!gMetalRenderPassDescriptor) {
        MLOG_WARNING(u8"Metal ImGui NewFrame skipped: no render pass descriptor available");
        return;
    }
    if (!gMetalCommandBuffer) {
        MLOG_WARNING(u8"Metal ImGui NewFrame skipped: no command buffer available");
        return;
    }

    ImGui_ImplMetal_NewFrame(gMetalRenderPassDescriptor);
    // TODO: Implement Metal ImGui new frame
}

void Device::ImguiShutdown() {
    MLOG_DEBUG(u8"Metal ImGui shutdown placeholder");

    // Only shutdown backend if it was initialized (ImGui_ImplMetal_Shutdown is safe to call
    // even if not initialized, but check to reduce log noise)
    if (gMetalDevice || gMetalCommandQueue) {
        ImGui_ImplMetal_Shutdown();
    }
    // TODO: Implement Metal ImGui shutdown
}

ShaderHandle Device::CreateShaderModule(const ShaderBytecodeView& bytecode) {
    MLOG_DEBUG(u8"Metal CreateShaderModule placeholder");
    // TODO: Implement Metal shader module creation
    ShaderHandle handle;
    handle.handle = 0;
    return handle;
}

PsoHandle Device::CreateRasterizePipeline(const RasterizePipelineDescriptor& desc) {
    MLOG_DEBUG(u8"Metal CreateRasterizePipeline placeholder");
    // TODO: Implement Metal pipeline state creation
    PsoHandle handle;
    handle.handle = 0;
    return handle;
}

// Swapchain implementation
void Swapchain::Initialize() {
    MLOG_DEBUG(u8"Initializing Metal Swapchain");
    
    if (!gMetalLayer) {
        MLOG_ERROR(u8"No Metal layer available for swapchain");
        return;
    }
    
    // Get window size
    unsigned int width, height;
    ll::os::gOS->GetActualWindowSize(width, height);
    
    // Configure the Metal layer
    gMetalLayer.pixelFormat = ConvertFormatToMetal(Format::BGRA8_UNORM);
    gMetalLayer.framebufferOnly = YES;
    gMetalLayer.drawableSize = CGSizeMake(width, height);
    
    // Set a test clear color (bright red for visibility)
    clearColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    MLOG_DEBUG(u8"Metal Swapchain - Set clear color to R:%.2f G:%.2f B:%.2f A:%.2f", 
               clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    
    MLOG_DEBUG(u8"Metal Swapchain initialized successfully");
}

void Swapchain::Shutdown() {
    MLOG_DEBUG(u8"Shutting down Metal Swapchain");
    
    if (gMetalCurrentDrawable) {
        gMetalCurrentDrawable = nil;
    }
    
    gMetalLayer = nil;
}

void* Swapchain::GetNativeHandle() {
    return (__bridge void*)gMetalLayer;
}

// Provide backend name for UI and diagnostics
namespace mercury { namespace ll { namespace graphics {
    const char* GetBackendName()
    {
        static const char* backendName = "METAL";
        return backendName;
    }
}}}

int Swapchain::GetWidth() const {
    if (gMetalLayer) {
        return static_cast<int>(gMetalLayer.drawableSize.width);
    }
    return 0;
}

int Swapchain::GetHeight() const {
    if (gMetalLayer) {
        return static_cast<int>(gMetalLayer.drawableSize.height);
    }
    return 0;
}

void Swapchain::Resize(mercury::u16 width, mercury::u16 height) {
    if (!gMetalLayer) {
        MLOG_ERROR(u8"Metal Swapchain - No Metal layer available for resize");
        return;
    }

    // Update the layer's drawable size. GetWidth/GetHeight read this value
    // directly from the CAMetalLayer, so we don't need to maintain separate
    // width/height members on the Swapchain object.
    gMetalLayer.drawableSize = CGSizeMake((CGFloat)width, (CGFloat)height);

    // Layer drawable size updated
}

CommandList Swapchain::AcquireNextImage() {
    // Acquire next image (Metal)
    CommandList cmdList;
    cmdList.nativePtr = nullptr;
    
    if (!gMetalLayer) {
        MLOG_ERROR(u8"Metal AcquireNextImage - No Metal layer available");
        return cmdList;
    }
    
    gMetalCurrentDrawable = [gMetalLayer nextDrawable];
    if (!gMetalCurrentDrawable) {
        MLOG_ERROR(u8"Metal AcquireNextImage - Failed to get drawable");
        return cmdList;
    }
    
    // Drawable obtained
    
    if (!gMetalCommandQueue) {
        MLOG_ERROR(u8"Metal AcquireNextImage - No command queue available");
        return cmdList;
    }
    
    // Creating command buffer
    gMetalCommandBuffer = [gMetalCommandQueue commandBuffer];
    if (!gMetalCommandBuffer) {
        MLOG_ERROR(u8"Metal AcquireNextImage - Failed to create command buffer");
        return cmdList;
    }
    
    // Creating render pass descriptor
    gMetalRenderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    if (!gMetalRenderPassDescriptor) {
        MLOG_ERROR(u8"Metal AcquireNextImage - Failed to create render pass descriptor");
        return cmdList;
    }
    
    // Setting up color attachment
    gMetalRenderPassColorAttachment = gMetalRenderPassDescriptor.colorAttachments[0];
    gMetalRenderPassColorAttachment.texture = gMetalCurrentDrawable.texture;
    gMetalRenderPassColorAttachment.loadAction = MTLLoadActionClear;
    gMetalRenderPassColorAttachment.storeAction = MTLStoreActionStore;

    // Set clear color from swapchain
    gMetalRenderPassColorAttachment.clearColor = MTLClearColorMake(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    // Clear color set
    
    // Creating render pass encoder
    gMetalRenderPassEncoder = [gMetalCommandBuffer renderCommandEncoderWithDescriptor:gMetalRenderPassDescriptor];
    if (!gMetalRenderPassEncoder) {
        MLOG_ERROR(u8"Metal AcquireNextImage - Failed to create render pass encoder");
        return cmdList;     
    }
    
    // Render pass encoder created
    cmdList.nativePtr = (__bridge void*)gMetalRenderPassEncoder;
    
    return cmdList;
}

void Swapchain::Present() {
    
    // End any active render encoders. There are two encoder globals used in
    // the engine/renderer: gMetalRenderPassEncoder (primary frame encoder)
    // and gMetalRenderEncoder (legacy/auxiliary). Ensure both are ended
    // to avoid Metal runtime assertions about encoders being deallocated
    // without endEncoding having been called.
    if (gMetalRenderPassEncoder) {
        [gMetalRenderPassEncoder endEncoding];
        gMetalRenderPassEncoder = nil;
    } else {
        // No render pass encoder to end
    }
    if (gMetalRenderEncoder) {
        [gMetalRenderEncoder endEncoding];
        gMetalRenderEncoder = nil;
    }
    
    if (gMetalCommandBuffer && gMetalCurrentDrawable) {
        [gMetalCommandBuffer presentDrawable:gMetalCurrentDrawable];
        [gMetalCommandBuffer commit];
        [gMetalCommandBuffer waitUntilCompleted];
        gMetalCommandBuffer = nil;
        gMetalCurrentDrawable = nil;
    } else {
        MLOG_ERROR(u8"Metal Present - Missing command buffer or drawable");
    }
}

// CommandList implementation
bool CommandList::IsExecuted() {
    if (gMetalCommandBuffer) {
        return [gMetalCommandBuffer status] == MTLCommandBufferStatusCompleted;
    }
    return false;
}

void CommandList::SetDebugName(const char* utf8_name) {
    if (gMetalCommandBuffer && utf8_name) {
        [gMetalCommandBuffer setLabel:[NSString stringWithUTF8String:utf8_name]];
    }
}

void CommandList::Destroy() {
    // Metal command lists are automatically managed
}

void CommandList::RenderImgui() {
    // ImGui render (Metal)

    // IMGUI_IMPL_API void ImGui_ImplMetal_RenderDrawData(ImDrawData* draw_data,
//                                                    MTL::CommandBuffer* commandBuffer,
//                                                    MTL::RenderCommandEncoder* commandEncoder);

    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), gMetalCommandBuffer, gMetalRenderPassEncoder);
    // TODO: Implement Metal ImGui rendering
}

void CommandList::SetPSO(Handle<u32> psoID) {
    // Set PSO placeholder
    // TODO: Implement Metal PSO setting
    // This would set the render pipeline state on the render pass encoder
}

void CommandList::Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) {
    // Draw placeholder
    // TODO: Implement Metal draw calls
    // This would call drawPrimitives on the render pass encoder
}

void CommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) {
    // Set viewport placeholder
    // TODO: Implement Metal viewport setting
    // This would call setViewport on the render pass encoder
}

void CommandList::SetScissor(i32 x, i32 y, u32 width, u32 height) {
    // Set scissor placeholder
    // TODO: Implement Metal scissor setting
    // This would call setScissorRect on the render pass encoder
}

// CommandPool implementation
CommandList CommandPool::AllocateCommandList() {
    CommandList cmdList;
    cmdList.nativePtr = nullptr; // Metal command lists don't need explicit allocation
    return cmdList;
}

void CommandPool::SetDebugName(const char* utf8_name) {
    if (gMetalCommandQueue && utf8_name) {
        [gMetalCommandQueue setLabel:[NSString stringWithUTF8String:utf8_name]];
    }
}

void CommandPool::Destroy() {
    // Metal command pools are automatically managed
}

void CommandPool::Reset() {
    // Metal command pools are automatically managed
}

#endif // MERCURY_LL_GRAPHICS_METAL
