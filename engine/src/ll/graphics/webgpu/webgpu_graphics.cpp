#include "ll/graphics.h"
#include "ll/os.h"
#include "mercury_application.h"



#if defined(MERCURY_LL_GRAPHICS_WEBGPU)

#ifdef MERCURY_LL_OS_WIN32
#pragma comment(lib, "D:\\Projects\\mercury\\engine\\third_party_prebuilt\\win64\\release\\lib\\webgpu_dawn.lib")
#endif

using namespace mercury;
using namespace mercury::ll::graphics;

#include "../../../imgui/imgui_impl.h"

#include "mercury_log.h"

#include "webgpu_utils.h"

/*
Move to OS
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/fetch.h>
*/

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// WebGPU objects
wgpu::Instance wgpuInstance;
wgpu::Adapter wgpuAdapter;
wgpu::Device wgpuDevice;
void *gWebGPUSurface = nullptr; // for emscripten os GetCurrentNativeWindowHandle
wgpu::Surface wgpuSurface;
wgpu::SurfaceTexture wgpuCurrentSwapchainTexture;

// State tracking
bool adapterRequested = false;
bool adapterReady = false;
bool deviceRequested = false;
bool deviceReady = false;

int gInitialCanvasWidth = 0;
int gInitialCanvasHeight = 0;

// Function to get canvas surface
/*
* MOVE to Emscripten OS
wgpu::Surface GetCanvasSurface()
{
    MLOG_DEBUG(u8"Getting canvas surface...");

    // Get the canvas element
    EMSCRIPTEN_RESULT result = emscripten_get_canvas_element_size("#canvas", &gInitialCanvasWidth, &gInitialCanvasHeight);
    if (result != EMSCRIPTEN_RESULT_SUCCESS)
    {
        MLOG_ERROR(u8"Failed to get canvas element");
        return nullptr;
    }

    // Create surface from canvas using Emscripten-specific types
    wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc;
    canvasDesc.selector = "canvas";

    wgpu::SurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = &canvasDesc;

    wgpu::Surface surface = wgpuInstance.CreateSurface(&surfaceDesc);
    if (surface)
    {
        MLOG_DEBUG(u8"Canvas surface created successfully");
    }
    else
    {
        MLOG_ERROR(u8"Failed to create canvas surface");
    }

    return surface;
}
*/

void Instance::Initialize()
{
    MLOG_DEBUG(u8"Initialize Graphics System (WEBGPU)");

    wgpu::InstanceDescriptor instanceDesc = {};
    instanceDesc.nextInChain = nullptr;
    instanceDesc.capabilities.nextInChain = nullptr;
    instanceDesc.capabilities.timedWaitAnyEnable = true;
    instanceDesc.capabilities.timedWaitAnyMaxCount = 64;
    wgpuInstance = wgpu::CreateInstance(&instanceDesc);
    wgpuSurface = *(wgpu::Surface*)os::gOS->GetWebGPUNativeWindowHandle();

    gWebGPUSurface = wgpuSurface.Get();

    if (wgpuInstance)
    {
        MLOG_DEBUG(u8"WebGPU instance created successfully");
    }
    else
    {
        MLOG_ERROR(u8"Failed to create WebGPU instance");
    }
}

void Instance::Shutdown()
{
    MLOG_DEBUG(u8"Shutdown Graphics System (WEBGPU)");

    wgpuDevice = nullptr;
    wgpuAdapter = nullptr;
    wgpuInstance = nullptr;

    adapterRequested = false;
    adapterReady = false;
    deviceRequested = false;
    deviceReady = false;
}

void *Instance::GetNativeHandle()
{
    return wgpuInstance.Get();
}

void Instance::AcquireAdapter(const AdapterSelectorInfo &selector_info)
{
    if (gAdapter == nullptr)
        gAdapter = new Adapter();
}

void Adapter::CreateDevice()
{
    if (gDevice == nullptr)
        gDevice = new Device();
}

void Adapter::Initialize()
{
    MLOG_DEBUG(u8"Initialize Adapter (WEBGPU)");

    auto graphicsConfig = mercury::Application::GetCurrentApplication()->GetConfig().graphics;

    if (!adapterRequested)
    {
        MLOG_DEBUG(u8"Starting WebGPU adapter request...");

        wgpu::RequestAdapterOptions adapterOptions = {};
        adapterOptions.nextInChain = nullptr;
        adapterOptions.compatibleSurface = wgpuSurface;
		//TODO: Implement power preference in config
        //adapterOptions.powerPreference = graphicsConfig.preferHighPerformance ? wgpu::PowerPreference::HighPerformance : wgpu::PowerPreference::LowPower;
        // Try with surface first
        wgpuInstance.RequestAdapter(&adapterOptions,
                                    wgpu::CallbackMode::AllowSpontaneous,
                                    [](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message)
                                    {
                                        MLOG_DEBUG(u8"RequestAdapter callback called with status: %d", (int)status);
                                        if (status == wgpu::RequestAdapterStatus::Success)
                                        {
                                            wgpuAdapter = adapter;
                                            MLOG_DEBUG(u8"WebGPU adapter acquired successfully");

                                            // Log adapter info
                                            wgpu::AdapterInfo adapterInfo;
                                            wgpu::Limits limits;

                                            wgpuAdapter.GetInfo(&adapterInfo);
                                            MLOG_DEBUG(u8"WebGPU adapter info:");

                                            MLOG_DEBUG(u8"  - Device: %s", adapterInfo.device.data);
                                            MLOG_DEBUG(u8"  - Vendor: %s", adapterInfo.vendor.data);
                                            MLOG_DEBUG(u8"  - Description: %s", adapterInfo.description.data);

                                            MLOG_DEBUG(u8"  - Device ID: %u", adapterInfo.deviceID);
                                            MLOG_DEBUG(u8"  - Vendor ID: %u", adapterInfo.vendorID);

                                            MLOG_DEBUG(u8"  - Backend type: %s (%d)", wgpu_utils::GetBackendTypeString(adapterInfo.backendType), (int)adapterInfo.backendType);
                                            MLOG_DEBUG(u8"  - Adapter type: %s (%d)", wgpu_utils::GetAdapterTypeString(adapterInfo.adapterType), (int)adapterInfo.adapterType);

                                            MLOG_DEBUG(u8"  - Subgroup Min Size: %u", adapterInfo.subgroupMinSize);
                                            MLOG_DEBUG(u8"  - Subgroup Max Size: %u", adapterInfo.subgroupMaxSize);

                                            wgpuAdapter.GetLimits(&limits);
                                            MLOG_DEBUG(u8"WebGPU adapter limits:");
                                            MLOG_DEBUG(u8"  - Max texture dimension 1D: %u", limits.maxTextureDimension1D);
                                            MLOG_DEBUG(u8"  - Max texture dimension 2D: %u", limits.maxTextureDimension2D);
                                            MLOG_DEBUG(u8"  - Max texture dimension 3D: %u", limits.maxTextureDimension3D);
                                            MLOG_DEBUG(u8"  - Max texture array layers: %u", limits.maxTextureArrayLayers);
                                            MLOG_DEBUG(u8"  - Max bind groups: %u", limits.maxBindGroups);
                                            MLOG_DEBUG(u8"  - Max dynamic uniform buffers per pipeline layout: %u", limits.maxDynamicUniformBuffersPerPipelineLayout);
                                            MLOG_DEBUG(u8"  - Max dynamic storage buffers per pipeline layout: %u", limits.maxDynamicStorageBuffersPerPipelineLayout);
                                            MLOG_DEBUG(u8"  - Max sampled textures per shader stage: %u", limits.maxSampledTexturesPerShaderStage);
                                            MLOG_DEBUG(u8"  - Max samplers per shader stage: %u", limits.maxSamplersPerShaderStage);
                                            MLOG_DEBUG(u8"  - Max storage buffers per shader stage: %u", limits.maxStorageBuffersPerShaderStage);
                                            MLOG_DEBUG(u8"  - Max storage textures per shader stage: %u", limits.maxStorageTexturesPerShaderStage);
                                            MLOG_DEBUG(u8"  - Max uniform buffers per shader stage: %u", limits.maxUniformBuffersPerShaderStage);
                                            MLOG_DEBUG(u8"  - Max uniform buffer binding size: %llu", limits.maxUniformBufferBindingSize);
                                            MLOG_DEBUG(u8"  - Max storage buffer binding size: %llu", limits.maxStorageBufferBindingSize);
                                            MLOG_DEBUG(u8"  - Max vertex buffers: %u", limits.maxVertexBuffers);
                                            MLOG_DEBUG(u8"  - Max vertex attributes: %u", limits.maxVertexAttributes);
                                            MLOG_DEBUG(u8"  - Max vertex buffer array stride: %u", limits.maxVertexBufferArrayStride);
                                            MLOG_DEBUG(u8"  - Max compute workgroup storage size: %u", limits.maxComputeWorkgroupStorageSize);
                                            MLOG_DEBUG(u8"  - Max compute invocations per workgroup: %u", limits.maxComputeInvocationsPerWorkgroup);
                                            MLOG_DEBUG(u8"  - Max compute workgroup size X: %u", limits.maxComputeWorkgroupSizeX);
                                            MLOG_DEBUG(u8"  - Max compute workgroup size Y: %u", limits.maxComputeWorkgroupSizeY);
                                            MLOG_DEBUG(u8"  - Max compute workgroup size Z: %u", limits.maxComputeWorkgroupSizeZ);
                                            MLOG_DEBUG(u8"  - Max compute workgroups per dimension: %u", limits.maxComputeWorkgroupsPerDimension);

                                            wgpu::SurfaceCapabilities surfaceCapabilities;
                                            wgpuSurface.GetCapabilities(wgpuAdapter, &surfaceCapabilities);
                                            MLOG_DEBUG(u8"WebGPU surface capabilities:");

                                            MLOG_DEBUG(u8"  - Supported texture usages:");
                                            if (surfaceCapabilities.usages & wgpu::TextureUsage::RenderAttachment)
                                            {
                                                MLOG_DEBUG(u8"   - Render Attachment");
                                            }
                                            if (surfaceCapabilities.usages & wgpu::TextureUsage::CopyDst)
                                            {
                                                MLOG_DEBUG(u8"   - Copy Destination");
                                            }

                                            MLOG_DEBUG(u8"  - Supported formats:");
                                            for (u32 i = 0; i < surfaceCapabilities.formatCount; ++i)
                                            {
                                                MLOG_DEBUG(u8"    - %s", wgpu_utils::GetTextureFormatString(surfaceCapabilities.formats[i]));
                                            }

                                            MLOG_DEBUG(u8"  - Supported present modes:");
                                            for (u32 i = 0; i < surfaceCapabilities.presentModeCount; ++i)
                                            {
                                                MLOG_DEBUG(u8"    - %s", wgpu_utils::GetPresentModeString(surfaceCapabilities.presentModes[i]));
                                            }

                                            MLOG_DEBUG(u8"  - Supported alpha modes:");
                                            for (u32 i = 0; i < surfaceCapabilities.alphaModeCount; ++i)
                                            {
                                                MLOG_DEBUG(u8"    - %s", wgpu_utils::GetCompositeAlphaModeString(surfaceCapabilities.alphaModes[i]));
                                            }
                                            adapterReady = true;
                                        }
                                        else
                                        {
                                            MLOG_ERROR(u8"Failed to acquire WebGPU adapter");
                                        }
                                    });

        MLOG_DEBUG(u8"RequestAdapter call completed, setting adapterRequested = true");
        adapterRequested = true;
    }

    // Wait for adapter to be ready
    MLOG_DEBUG(u8"Waiting for WebGPU adapter...");
    int waitCount = 0;
    while (!adapterReady)
    {
        ll::os::gOS->Sleep(10);
        waitCount++;
        if (waitCount % 100 == 0)
        { // Log every 1 second (100 * 10ms)
            MLOG_DEBUG(u8"Still waiting for adapter... (waited %d ms)", waitCount * 10);
        }
    }
}

void Adapter::Shutdown()
{
    MLOG_DEBUG(u8"Shutdown Adapter (WEBGPU)");
}

void *Adapter::GetNativeHandle()
{
    return wgpuAdapter.Get();
}

void Device::Initialize()
{
    MLOG_DEBUG(u8"Initialize Device (WEBGPU)");

    if (!deviceRequested)
    {
        MLOG_DEBUG(u8"Starting WebGPU device request...");

        wgpu::DeviceDescriptor deviceDesc = {};
        deviceDesc.nextInChain = nullptr;
        deviceDesc.label = "Mercury WebGPU Device";

        MLOG_DEBUG(u8"Calling wgpuAdapter.RequestDevice...");
        wgpuAdapter.RequestDevice(&deviceDesc,
                                  wgpu::CallbackMode::AllowSpontaneous,
                                  [](wgpu::RequestDeviceStatus status, wgpu::Device device, wgpu::StringView message)
                                  {
                                      MLOG_DEBUG(u8"RequestDevice callback called with status: %d", (int)status);
                                      if (status == wgpu::RequestDeviceStatus::Success)
                                      {
                                          wgpuDevice = device;
                                          MLOG_DEBUG(u8"WebGPU device created successfully");

                                          MLOG_DEBUG(u8"WebGPU device configured");
                                          deviceReady = true;
                                      }
                                      else
                                      {
                                          MLOG_ERROR(u8"Failed to create WebGPU device");
                                      }
                                  });

        MLOG_DEBUG(u8"RequestDevice call completed, setting deviceRequested = true");
        deviceRequested = true;
    }

    // Wait for device to be ready
    MLOG_DEBUG(u8"Waiting for WebGPU device...");
    int waitCount = 0;
    while (!deviceReady)
    {
        ll::os::gOS->Sleep(10);
        waitCount++;
        if (waitCount % 100 == 0)
        { // Log every 1 second (100 * 10ms)
            MLOG_DEBUG(u8"Still waiting for device... (waited %d ms)", waitCount * 10);
        }
    }
    MLOG_DEBUG(u8"Device ready! Total wait time: %d ms", waitCount * 10);
}

void Device::Shutdown()
{
    MLOG_DEBUG(u8"Shutdown Device (WEBGPU)");
}

void Device::Tick()
{
}

void Device::ImguiInitialize()
{
    ImGui_ImplWGPU_InitInfo initInfo = {};
    initInfo.Device = wgpuDevice.Get();
    initInfo.RenderTargetFormat = WGPUTextureFormat_RGBA8Unorm;
    initInfo.NumFramesInFlight = 3;
    initInfo.DepthStencilFormat = WGPUTextureFormat::WGPUTextureFormat_Undefined;

    ImGui_ImplWGPU_Init(&initInfo);
}

void *Device::GetNativeHandle()
{
    return wgpuDevice.Get();
}

void Device::InitializeSwapchain()
{
    assert(gSwapchain == nullptr && "Swapchain already initialized");

    MLOG_DEBUG(u8"Initializing WebGPU swapchain...");

    gSwapchain = new Swapchain();
    gSwapchain->Initialize();
}

void Device::ShutdownSwapchain()
{
    assert(gSwapchain != nullptr && "Swapchain is not initialized");

    MLOG_DEBUG(u8"Shutting down WebGPU swapchain...");

    gSwapchain->Shutdown();
    delete gSwapchain;
    gSwapchain = nullptr;
}

void Swapchain::Initialize()
{
    MLOG_DEBUG(u8"Initializing Swapchain (WEBGPU)");

    wgpu::SurfaceConfiguration surfaceConfig = {};
    surfaceConfig.nextInChain = nullptr;
    surfaceConfig.format = wgpu::TextureFormat::BGRA8Unorm;
    surfaceConfig.usage = wgpu::TextureUsage::RenderAttachment;
    surfaceConfig.presentMode = wgpu::PresentMode::Fifo;
    surfaceConfig.width = 1024;
    surfaceConfig.height = 576;
    surfaceConfig.device = wgpuDevice;
    surfaceConfig.alphaMode = wgpu::CompositeAlphaMode::Opaque;

    wgpuSurface.Configure(&surfaceConfig);
    
    MLOG_DEBUG(u8"Swapchain initialized with format: %s, usage: %d, present mode: %s, size: %dx%d",
               wgpu_utils::GetTextureFormatString(surfaceConfig.format),
               (int)surfaceConfig.usage,
               wgpu_utils::GetPresentModeString(surfaceConfig.presentMode),
               surfaceConfig.width,
               surfaceConfig.height);
}

void Swapchain::Shutdown()
{
    MLOG_DEBUG(u8"Shutting down Swapchain (WEBGPU)");
}

void *Swapchain::GetNativeHandle()
{
    return wgpuSurface.Get(); // WebGPU does not expose a native handle for swapchains
}

CommandList Swapchain::AcquireNextImage()
{
    wgpuSurface.GetCurrentTexture(&wgpuCurrentSwapchainTexture);

    //TODO: get
    CommandList clist = {};
    return clist;
}

void ClearTextureWithColor(float r, float g, float b, float a)
{
    wgpu::CommandEncoder commandEncoder = wgpuDevice.CreateCommandEncoder();

    // Create a render pass descriptor
    wgpu::RenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = wgpuCurrentSwapchainTexture.texture.CreateView();
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = {r, g, b, a};

    wgpu::RenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;

    // Begin the render pass
    wgpu::RenderPassEncoder renderPass = commandEncoder.BeginRenderPass(&renderPassDesc);

    renderPass.End();

    // Finish encoding and submit the commands
    wgpu::CommandBuffer commandBuffer = commandEncoder.Finish();
    wgpuDevice.GetQueue().Submit(1, &commandBuffer);
}

void Swapchain::Present()
{
    //static float a = 0.0f;
   // a += 0.01f;

    //ClearTextureWithColor(sin(a) * 0.5f + 0.5f, cos(a) * 0.5f + 0.5f, 0.5f, 1.0f);
  
    ll::os::gOS->WebGPUPresent();

    /*
    #ifndef MERCURY_LL_OS_EMSCRIPTEN
    wgpuSurface.Present();
    #endif
    */
}


void CommandList::RenderImgui()
{
    //auto cmdList = static_cast<WGPURenderPassEncoder>(ctx.impl);
    //ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), cmdList);
}


#endif //MERCURY_LL_GRAPHICS_WEBGPU