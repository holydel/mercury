#include "ll/graphics.h"
#include "ll/os.h"
#include "mercury_application.h"



#if defined(MERCURY_LL_GRAPHICS_WEBGPU)

#ifdef MERCURY_LL_OS_WIN32
#ifdef _NDEBUG
#pragma comment(lib, "D:\\Projects\\mercury\\engine\\third_party_prebuilt\\win64\\debug\\lib\\webgpu_dawn.lib")
#else
#pragma comment(lib, "D:\\Projects\\mercury\\engine\\third_party_prebuilt\\win64\\release\\lib\\webgpu_dawn.lib")
#endif
#endif

using namespace mercury;
using namespace mercury::ll::graphics;

bool mercury::ll::graphics::IsYFlipped()
{
    return true;
}

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
wgpu::TextureFormat wgpuSwapchainFormat = wgpu::TextureFormat::BGRA8Unorm;
wgpu::Sampler wgpuDefaultLinearSampler;
wgpu::Sampler wgpuDefaultNearestSampler;
wgpu::Sampler wgpuDefaultTrilinearSampler;
wgpu::Sampler wgpuDefaultAnisotropicSampler;

// State tracking
bool adapterRequested = false;
bool adapterReady = false;
bool deviceRequested = false;
bool deviceReady = false;

int gInitialCanvasWidth = 0;
int gInitialCanvasHeight = 0;
int gCurrentCanvasWidth = 0;
int gCurrentCanvasHeight = 0;

// Global storage for shaders and pipelines
std::vector<wgpu::ShaderModule> gAllShaderModules;

struct PSOMeta
{
	wgpu::PipelineLayout pipelineLayout;
    bool hasPushConstants = false;
};

std::vector<wgpu::RenderPipeline> gAllPSOs;
std::vector<PSOMeta> gAllPSOMetas;
std::vector<wgpu::BindGroupLayout> gAllPSOLayouts;
std::vector<wgpu::Buffer> gAllBuffers;
std::vector<wgpu::BindGroupLayout> gAllParameterBlockLayouts;
std::vector<wgpu::BindGroup> gAllParameterBlocks;

struct ParamaterBllockMeta
{
	ParameterBlockLayoutHandle layoutHandle;
};

std::vector<ParamaterBllockMeta> gAllParameterBlockMetas;

struct TextureMeta
{
	wgpu::TextureView textureView;
};

std::vector<TextureMeta> gAllTextureMetas;
std::vector<wgpu::Texture> gAllTextures;

struct PerFrameData
{
	u8* pushConstantData;
    u32 pushConstantOffset;
    wgpu::Buffer pushConstantBuffer;
	wgpu::BindGroup pushConstantBindGroup;

};
std::vector<PerFrameData> gPerFrameData;
wgpu::BindGroupLayout gPushConstantBindGroupLayout;

int gNumFramesInFlight = 2;
int gCurrentFrameIndex = 0;

void Instance::Initialize()
{
    MLOG_DEBUG(u8"Initialize Graphics System (WEBGPU)");
    
    #ifdef MERCURY_LL_OS_WIN32
    // Set up Dawn procedure table before creating any WebGPU objects
    SetDllDirectoryA("C:\\Windows\\System32");
    // dawnProcSetProcs(&dawn::native::GetProcs());
    #endif  // __EMSCRIPTEN__

        wgpu::InstanceDescriptor instanceDesc = {};
        instanceDesc.nextInChain = nullptr;
    instanceDesc.capabilities.nextInChain = nullptr;
    instanceDesc.capabilities.timedWaitAnyEnable = true;
    instanceDesc.capabilities.timedWaitAnyMaxCount = 64;

#ifndef MERCURY_LL_OS_EMSCRIPTEN
    wgpu::DawnTogglesDescriptor togglesDesc = {};
    const char* enabledToggles[] = { "allow_unsafe_apis" };
    togglesDesc.nextInChain = nullptr;
    togglesDesc.enabledToggles = enabledToggles;
    togglesDesc.enabledToggleCount = 1;
    
    instanceDesc.nextInChain = &togglesDesc;
#endif

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

    gAllShaderModules.clear();
    gAllPSOs.clear();
    
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
        adapterOptions.featureLevel = wgpu::FeatureLevel::Core;
        adapterOptions.backendType = wgpu::BackendType::Vulkan;
        auto graphicsConfig = mercury::Application::GetCurrentApplication()->GetConfig().graphics;
        //adapterOptions.powerPreference = graphicsConfig.adapterPreference == Config::Graphics::AdapterTypePreference::HighPerformance ? wgpu::PowerPreference::HighPerformance : wgpu::PowerPreference::LowPower;
        // Try with surface first
       
        wgpuInstance.RequestAdapter(&adapterOptions, 
                                    wgpu::CallbackMode::AllowSpontaneous,
                                    [this](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView)
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

											wgpuSwapchainFormat = surfaceCapabilities.formats[0];
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
        wgpuInstance.ProcessEvents();
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
        
        deviceDesc.SetDeviceLostCallback(
            wgpu::CallbackMode::AllowSpontaneous,
            [](const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView message) {
                const char* reasonName = "";
                switch (reason) {
                case wgpu::DeviceLostReason::Unknown:
                    reasonName = "Unknown";
                    break;
                case wgpu::DeviceLostReason::Destroyed:
                    reasonName = "Destroyed";
                    break;
                case wgpu::DeviceLostReason::CallbackCancelled:
                    reasonName = "CallbackCancelled";
                    break;
                case wgpu::DeviceLostReason::FailedCreation:
                    reasonName = "FailedCreation";
                    break;
                }
                MLOG_FATAL(u8"Device lost because of %s: %s\n", reasonName, message.data);
            });
        deviceDesc.SetUncapturedErrorCallback(
            [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message) {
                const char* errorTypeName = "";
                switch (type) {
                case wgpu::ErrorType::Validation:
                    errorTypeName = "Validation";
                    break;
                case wgpu::ErrorType::OutOfMemory:
                    errorTypeName = "Out of memory";
                    break;
                case wgpu::ErrorType::Internal:
                    errorTypeName = "Internal";
                    break;
                case wgpu::ErrorType::Unknown:
                    errorTypeName = "Unknown";
                    break;
                }
                MLOG_ERROR(u8"%s error: %s\n", errorTypeName, message.data);
            });

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

    std::vector<wgpu::BindGroupLayoutEntry> bglEntries;
    wgpu::BindGroupLayoutEntry pcEntry{};
    pcEntry.binding = 0;
    pcEntry.buffer.hasDynamicOffset = true;
    pcEntry.buffer.type = wgpu::BufferBindingType::Uniform;
    pcEntry.buffer.minBindingSize = 256;
    pcEntry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
    bglEntries.push_back(pcEntry);

	wgpu::BindGroupLayoutDescriptor bglDesc{};
	bglDesc.entryCount = static_cast<uint32_t>(bglEntries.size());
	bglDesc.entries = bglEntries.data();
	bglDesc.label = "Push Constant Bind Group Layout";
	gPushConstantBindGroupLayout = wgpuDevice.CreateBindGroupLayout(&bglDesc);

	u32 pushConstantsDataSize = 65536; // max uniform buffer size by spec
	gPerFrameData.resize(gNumFramesInFlight);

    for (int i = 0; i < gNumFramesInFlight; ++i)
    {
		PerFrameData& frameData = gPerFrameData[i];

		frameData.pushConstantData = new u8[pushConstantsDataSize];

        wgpu::BufferDescriptor pcBufferDesc{};
		pcBufferDesc.size = pushConstantsDataSize;
        pcBufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        pcBufferDesc.mappedAtCreation = false;
		pcBufferDesc.label = "Push Constant Buffer";
        
        frameData.pushConstantBuffer = wgpuDevice.CreateBuffer(&pcBufferDesc);

		wgpu::BindGroupEntry pcBindGroupEntry{};
		pcBindGroupEntry.binding = 0;
		pcBindGroupEntry.buffer = frameData.pushConstantBuffer;
        pcBindGroupEntry.size = 256;

		wgpu::BindGroupDescriptor pcBindGroupDesc{};
		pcBindGroupDesc.layout = gPushConstantBindGroupLayout;
		pcBindGroupDesc.entryCount = 1;
		pcBindGroupDesc.entries = &pcBindGroupEntry;
		pcBindGroupDesc.label = "Push Constant Bind Group";
        frameData.pushConstantBindGroup = wgpuDevice.CreateBindGroup(&pcBindGroupDesc);
    }

    {
		wgpu::SamplerDescriptor samplerDesc{};
		samplerDesc.addressModeU = wgpu::AddressMode::Repeat;
		samplerDesc.addressModeV = wgpu::AddressMode::Repeat;
		samplerDesc.addressModeW = wgpu::AddressMode::Repeat;
		samplerDesc.magFilter = wgpu::FilterMode::Linear;
		samplerDesc.minFilter = wgpu::FilterMode::Linear;
		samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Nearest;
        samplerDesc.label = "Default Linear Sampler";
        wgpuDefaultLinearSampler = wgpuDevice.CreateSampler(&samplerDesc);
    }

    {
        wgpu::SamplerDescriptor samplerDesc{};
        samplerDesc.addressModeU = wgpu::AddressMode::Repeat;
        samplerDesc.addressModeV = wgpu::AddressMode::Repeat;
        samplerDesc.addressModeW = wgpu::AddressMode::Repeat;
        samplerDesc.magFilter = wgpu::FilterMode::Nearest;
        samplerDesc.minFilter = wgpu::FilterMode::Nearest;
        samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Nearest;
        samplerDesc.label = "Default Nearest Sampler";
        wgpuDefaultNearestSampler = wgpuDevice.CreateSampler(&samplerDesc);
    }
    
    {
        wgpu::SamplerDescriptor samplerDesc{};
        samplerDesc.addressModeU = wgpu::AddressMode::Repeat;
        samplerDesc.addressModeV = wgpu::AddressMode::Repeat;
        samplerDesc.addressModeW = wgpu::AddressMode::Repeat;
        samplerDesc.magFilter = wgpu::FilterMode::Linear;
        samplerDesc.minFilter = wgpu::FilterMode::Linear;
        samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
        samplerDesc.label = "Default Trilinear Sampler";
        wgpuDefaultTrilinearSampler = wgpuDevice.CreateSampler(&samplerDesc);
    }

    {
        wgpu::SamplerDescriptor samplerDesc{};
        samplerDesc.addressModeU = wgpu::AddressMode::Repeat;
        samplerDesc.addressModeV = wgpu::AddressMode::Repeat;
        samplerDesc.addressModeW = wgpu::AddressMode::Repeat;
        samplerDesc.magFilter = wgpu::FilterMode::Linear;
        samplerDesc.minFilter = wgpu::FilterMode::Linear;
        samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
        samplerDesc.maxAnisotropy = 16;
        samplerDesc.label = "Default Anisotropic Sampler";
		wgpuDefaultAnisotropicSampler = wgpuDevice.CreateSampler(&samplerDesc);
    }
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
    MLOG_DEBUG(u8"Initialize ImGui (WEBGPU)");
    ImGui_ImplWGPU_InitInfo initInfo = {};
    initInfo.Device = wgpuDevice.Get();
    initInfo.RenderTargetFormat = (WGPUTextureFormat)wgpuSwapchainFormat;
    initInfo.NumFramesInFlight = 2;
    initInfo.DepthStencilFormat = WGPUTextureFormat::WGPUTextureFormat_Undefined;

    ImGui_ImplWGPU_Init(&initInfo);
}

void Device::ImguiRegenerateFontAtlas()
{
    ImGui_ImplWGPU_CreateDeviceObjects();
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

void Swapchain::ReInitIfNeeded()
{
    unsigned int currentWidth, currentHeight;
    ll::os::gOS->GetActualWindowSize(currentWidth, currentHeight);
    
    if (currentWidth != gCurrentCanvasWidth || currentHeight != gCurrentCanvasHeight)
    {
        MLOG_DEBUG(u8"Canvas size changed from %dx%d to %dx%d, resizing swapchain", 
                   gCurrentCanvasWidth, gCurrentCanvasHeight, currentWidth, currentHeight);
        Resize(currentWidth, currentHeight);
    }
}

void Swapchain::Initialize()
{
    MLOG_DEBUG(u8"Initializing Swapchain (WEBGPU)");

	ll::os::gOS->GetActualWindowSize((unsigned int&)gInitialCanvasWidth, (unsigned int&)gInitialCanvasHeight);
    gCurrentCanvasWidth = gInitialCanvasWidth;
    gCurrentCanvasHeight = gInitialCanvasHeight;

    wgpu::SurfaceConfiguration surfaceConfig = {};
    surfaceConfig.nextInChain = nullptr;
    surfaceConfig.format = wgpuSwapchainFormat;
    surfaceConfig.usage = wgpu::TextureUsage::RenderAttachment;
    surfaceConfig.presentMode = wgpu::PresentMode::Fifo;
    surfaceConfig.width = gInitialCanvasWidth;
    surfaceConfig.height = gInitialCanvasHeight;
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

void Swapchain::Resize(u16 width, u16 height)
{
    MLOG_DEBUG(u8"Resizing Swapchain (WEBGPU) to %dx%d", width, height);
    wgpu::SurfaceConfiguration surfaceConfig = {};
    surfaceConfig.nextInChain = nullptr;
    surfaceConfig.format = wgpuSwapchainFormat;
    surfaceConfig.usage = wgpu::TextureUsage::RenderAttachment;
    surfaceConfig.presentMode = wgpu::PresentMode::Fifo;
    surfaceConfig.width = width;
    surfaceConfig.height = height;
    surfaceConfig.device = wgpuDevice;
    surfaceConfig.alphaMode = wgpu::CompositeAlphaMode::Opaque;
    wgpuSurface.Configure(&surfaceConfig);
    gCurrentCanvasWidth = width;
    gCurrentCanvasHeight = height;
    MLOG_DEBUG(u8"Swapchain resized with format: %s, usage: %d, present mode: %s, size: %dx%d",
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

wgpu::CommandEncoder gCurrentCommandEncoder;
wgpu::RenderPassEncoder gCurrentFinalRenderPass;

CommandList Swapchain::AcquireNextImage()
{
    // Check if swapchain needs to be resized
    ReInitIfNeeded();
    
    wgpuSurface.GetCurrentTexture(&wgpuCurrentSwapchainTexture);

    gCurrentCommandEncoder = wgpuDevice.CreateCommandEncoder();
	gCurrentCommandEncoder.SetLabel("Final Render Pass Command Encoder");
    // Create a render pass descriptor
    wgpu::RenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = wgpuCurrentSwapchainTexture.texture.CreateView();
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };

    wgpu::RenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;

    // Begin the render pass
    gCurrentFinalRenderPass = gCurrentCommandEncoder.BeginRenderPass(&renderPassDesc);

	gPerFrameData[gCurrentFrameIndex].pushConstantOffset = 0;

    CommandList clist = { &gCurrentCommandEncoder };
	clist.currentRenderPassNativePtr = &gCurrentFinalRenderPass;
    return clist;
}

void Swapchain::Present()
{
    gCurrentFinalRenderPass.End();

    // Finish encoding and submit the commands
    wgpu::CommandBuffer commandBuffer = gCurrentCommandEncoder.Finish();


    wgpuDevice.GetQueue().WriteBuffer(
        gPerFrameData[gCurrentFrameIndex].pushConstantBuffer,
        0,
        gPerFrameData[gCurrentFrameIndex].pushConstantData,
		gPerFrameData[gCurrentFrameIndex].pushConstantOffset);

    wgpuDevice.GetQueue().Submit(1, &commandBuffer);

    ll::os::gOS->WebGPUPresent();
    //wgpuSurface.re
   // wgpuCurrentSwapchainTexture.Present();
    //if (wgpuCurrentSwapchainTexture) {
    //    wgpuCurrentSwapchainTexture.Present();   // release/present the acquired texture
    //    wgpuCurrentSwapchainTexture = nullptr;   // clear handle
    //}
    //wgpuCurrentSwapchainTexture.texture.
    unsigned int currentWidth = 0;
    unsigned int currentHeight = 0;
    ll::os::gOS->GetActualWindowSize(currentWidth, currentHeight);
    if (currentWidth != gInitialCanvasWidth || currentHeight != gInitialCanvasHeight)
    {
        gInitialCanvasWidth = currentWidth;
        gInitialCanvasHeight = currentHeight;
        gCurrentCanvasWidth = currentWidth;
        gCurrentCanvasHeight = currentHeight;
        Resize(gInitialCanvasWidth, gInitialCanvasHeight);
    }

	gCurrentFrameIndex = (gCurrentFrameIndex + 1) % gNumFramesInFlight;
}


void CommandList::RenderImgui()
{
   ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), gCurrentFinalRenderPass.Get());
}

const char* ll::graphics::GetBackendName()
{
    static const char* backendName = "WebGPU";
    return backendName;
}

// CommandList implementations
void CommandList::SetPSO(PsoHandle psoID)
{
	if (currentPsoID.handle == psoID.handle)
        return;

    currentPsoID = psoID;

    gCurrentFinalRenderPass.SetPipeline(gAllPSOs[psoID.handle]); 
}

void CommandList::Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
{
    gCurrentFinalRenderPass.Draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
    gCurrentFinalRenderPass.SetViewport(x, y, width, height, minDepth, maxDepth);
}

void CommandList::SetScissor(i32 x, i32 y, u32 width, u32 height)
{
    gCurrentFinalRenderPass.SetScissorRect(x, y, width, height);

}

// Device implementations
void Device::ImguiNewFrame()
{
    ImGui_ImplWGPU_NewFrame();
}

void Device::ImguiShutdown()
{
    ImGui_ImplWGPU_Shutdown();
}

ShaderHandle Device::CreateShaderModule(const ShaderBytecodeView& bytecode)
{
    ShaderHandle result;
    result.handle = static_cast<u32>(gAllShaderModules.size());

    wgpu::ShaderSourceWGSL wgslDesc{};
    wgslDesc.code = static_cast<const char*>(bytecode.data);
    wgslDesc.nextInChain = nullptr;
    wgslDesc.sType = wgpu::SType::ShaderSourceWGSL;

    wgpu::ShaderModuleDescriptor desc{};
    desc.nextInChain = &wgslDesc;

    auto module = wgpuDevice.CreateShaderModule(&desc);
    gAllShaderModules.push_back(module);

    return result;
}

void Device::UpdateShaderModule(ShaderHandle shaderModuleID, const ShaderBytecodeView& bytecode)
{
    // Release old module
    gAllShaderModules[shaderModuleID.handle] = nullptr;

    wgpu::ShaderSourceWGSL wgslDesc{};
    wgslDesc.code = static_cast<const char*>(bytecode.data);
    wgslDesc.nextInChain = nullptr;
    wgslDesc.sType = wgpu::SType::ShaderSourceWGSL;

    wgpu::ShaderModuleDescriptor desc{};
    desc.nextInChain = &wgslDesc;

    auto module = wgpuDevice.CreateShaderModule(&desc);
    gAllShaderModules[shaderModuleID.handle] = module;
}

void Device::DestroyShaderModule(ShaderHandle shaderModuleID)
{
    gAllShaderModules[shaderModuleID.handle] = nullptr;
}

wgpu::PrimitiveTopology primitiveTopologyFromLLTopology(PrimitiveTopology topology)
{
    switch (topology)
    {
    case PrimitiveTopology::TriangleList:
        return wgpu::PrimitiveTopology::TriangleList;
    case PrimitiveTopology::TriangleStrip:
        return wgpu::PrimitiveTopology::TriangleStrip;
    case PrimitiveTopology::LineList:
        return wgpu::PrimitiveTopology::LineList;
    case PrimitiveTopology::LineStrip:
        return wgpu::PrimitiveTopology::LineStrip;
    case PrimitiveTopology::PointList:
        return wgpu::PrimitiveTopology::PointList;
    default:
        return wgpu::PrimitiveTopology::TriangleList;
    }
}

PsoHandle Device::CreateRasterizePipeline(const RasterizePipelineDescriptor& desc)
{
    MLOG_DEBUG(u8"Create Rasterize Pipeline (WEBGPU)");
    PsoHandle result;
    result.handle = static_cast<u32>(gAllPSOs.size());

    wgpu::RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.nextInChain = nullptr;
    

    // Vertex stage
    wgpu::VertexState vertexState{};
    if (desc.vertexShader.isValid())
    {
        vertexState.module = gAllShaderModules[desc.vertexShader.handle];
        vertexState.entryPoint = "main";
    }
    pipelineDesc.vertex = vertexState;

    // Fragment stage
    wgpu::FragmentState fragmentState{};
    wgpu::ColorTargetState colorTarget{};
    colorTarget.format = wgpuSwapchainFormat;

    if (desc.fragmentShader.isValid())
    {
        fragmentState.module = gAllShaderModules[desc.fragmentShader.handle];
        fragmentState.entryPoint = "main";
        fragmentState.targets = &colorTarget;
        fragmentState.targetCount = 1;
    }

    pipelineDesc.fragment = &fragmentState;

    // Primitive state
    wgpu::PrimitiveState primitiveState{};
    primitiveState.topology = primitiveTopologyFromLLTopology(desc.primitiveTopology);
    pipelineDesc.primitive = primitiveState;

    // Multisample state
    wgpu::MultisampleState multisampleState{};
    multisampleState.count = 1;
    multisampleState.mask = 0xFFFFFFFF;
    pipelineDesc.multisample = multisampleState;

	
	auto& psoMeta = gAllPSOMetas.emplace_back();

	std::vector<wgpu::BindGroupLayout> bgls;
    
    if (desc.pushConstantSize > 0)
    {
		psoMeta.hasPushConstants = true;
		bgls.push_back(gPushConstantBindGroupLayout);
    }

    for (auto& bg : desc.bindingSetLayouts)
    {
		if (bg.allSlots.empty())
            continue;

        std::vector<wgpu::BindGroupLayoutEntry> bglEntries;
        wgpu::BindGroupLayoutEntry pcEntry{};

		u32 bindingIndex = 0;

        for (int i = 0; i < bg.allSlots.size(); ++i)
        {
            auto& slot = bg.allSlots[i];

            if (slot.resourceType == ShaderResourceType::UniformBuffer)
            {
                wgpu::BindGroupLayoutEntry entry{};
                entry.binding = static_cast<u32>(bindingIndex);
                entry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                entry.buffer.hasDynamicOffset = false;
                entry.buffer.type = wgpu::BufferBindingType::Uniform;
                bglEntries.push_back(entry);
                bindingIndex++;
            }

            if (slot.resourceType == ShaderResourceType::SampledImage2D)
            {
                {
                    wgpu::BindGroupLayoutEntry entry{};
                    entry.binding = static_cast<u32>(bindingIndex);
                    entry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entry.texture.sampleType = wgpu::TextureSampleType::Float;
                    entry.texture.viewDimension = wgpu::TextureViewDimension::e2D;
                    bglEntries.push_back(entry);
                    ++bindingIndex;
                }

                {
                    wgpu::BindGroupLayoutEntry entry{};
                    entry.binding = static_cast<u32>(bindingIndex);
                    entry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entry.sampler.type = wgpu::SamplerBindingType::Filtering;
                    bglEntries.push_back(entry);
                }
            }
        }

        wgpu::BindGroupLayoutDescriptor bglDesc{};
        bglDesc.entryCount = static_cast<uint32_t>(bglEntries.size());
        bglDesc.entries = bglEntries.data();
        bglDesc.label = "Scene Group Layout";

		auto bgl = wgpuDevice.CreateBindGroupLayout(&bglDesc);
		bgls.push_back(bgl);
    }


    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc{};

    pipelineLayoutDesc.bindGroupLayouts = bgls.data();
	pipelineLayoutDesc.bindGroupLayoutCount = static_cast<u32>(bgls.size());
    psoMeta.pipelineLayout = wgpuDevice.CreatePipelineLayout(&pipelineLayoutDesc);
    pipelineDesc.layout = psoMeta.pipelineLayout;
    auto pipeline = wgpuDevice.CreateRenderPipeline(&pipelineDesc);
    gAllPSOs.push_back(pipeline);

    return result;
}

void Device::UpdatePipelineState(PsoHandle psoID, const RasterizePipelineDescriptor& desc)
{
    // Release old pipeline
    gAllPSOs[psoID.handle] = nullptr;

    wgpu::RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.nextInChain = nullptr;

    // Vertex stage
    wgpu::VertexState vertexState{};
    if (desc.vertexShader.isValid())
    {
        vertexState.module = gAllShaderModules[desc.vertexShader.handle];
        vertexState.entryPoint = "main";
    }
    pipelineDesc.vertex = vertexState;

    // Fragment stage
    wgpu::FragmentState fragmentState{};
    if (desc.fragmentShader.isValid())
    {
        fragmentState.module = gAllShaderModules[desc.fragmentShader.handle];
        fragmentState.entryPoint = "main";

        wgpu::ColorTargetState colorTarget{};
        colorTarget.format = wgpuSwapchainFormat;
        fragmentState.targets = &colorTarget;
        fragmentState.targetCount = 1;
    }
    pipelineDesc.fragment = &fragmentState;

    // Primitive state
    wgpu::PrimitiveState primitiveState{};
    primitiveState.topology = wgpu::PrimitiveTopology::TriangleList;
    pipelineDesc.primitive = primitiveState;

    // Multisample state
    wgpu::MultisampleState multisampleState{};
    multisampleState.count = 1;
    multisampleState.mask = 0xFFFFFFFF;
    pipelineDesc.multisample = multisampleState;

    auto pipeline = wgpuDevice.CreateRenderPipeline(&pipelineDesc);
    gAllPSOs[psoID.handle] = pipeline;
}

void Device::DestroyRasterizePipeline(PsoHandle psoID)
{
    gAllPSOs[psoID.handle] = nullptr;
}


// Swapchain implementations
int Swapchain::GetWidth() const
{
    return gCurrentCanvasWidth;
}

int Swapchain::GetHeight() const
{
    return gCurrentCanvasHeight;
}

void CommandList::PushConstants(const void* data, size_t size)
{
    auto& psoMeta = gAllPSOMetas[currentPsoID.handle];

    if (psoMeta.hasPushConstants)
    {
        auto& frame = gPerFrameData[gCurrentFrameIndex];

        auto rpassEnc = (wgpu::RenderPassEncoder*)currentRenderPassNativePtr;
        rpassEnc->SetBindGroup(0, frame.pushConstantBindGroup, 1, &frame.pushConstantOffset);
		memcpy(frame.pushConstantData + frame.pushConstantOffset, data, size);
		frame.pushConstantOffset += 256;
    }
}


void Device::DestroyBuffer(BufferHandle bufferID)
{
    // Implementation for destroying a buffer
}

void Device::UpdateBuffer(BufferHandle bufferID, const void* data, size_t size, size_t offset)
{
    wgpuDevice.GetQueue().WriteBuffer(
        gAllBuffers[bufferID.handle],
        static_cast<u64>(offset),
        data,
		static_cast<u64>(size));
}

BufferHandle Device::CreateBuffer(const BufferDescriptor& desc)
{
	wgpu::BufferDescriptor bufferDesc{};
	bufferDesc.size = static_cast<u64>(desc.size);
	bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
	bufferDesc.mappedAtCreation = false;
	auto buffer = wgpuDevice.CreateBuffer(&bufferDesc);
	gAllBuffers.push_back(buffer);

    BufferHandle h;
    h.handle = static_cast<u32>(gAllBuffers.size() - 1);
    return h;
}

ParameterBlockLayoutHandle Device::CreateParameterBlockLayout(const BindingSetLayoutDescriptor& layoutDesc, int setIndex)
{
    wgpu::BindGroupLayoutDescriptor desc{};
	desc.label = "Parameter Block Layout";

	std::vector<wgpu::BindGroupLayoutEntry> entries;
	int bindingIndex = 0;

	for (int i = 0; i < layoutDesc.allSlots.size(); ++i)
    {
		auto& slot = layoutDesc.allSlots[i];

        if (slot.resourceType == ShaderResourceType::UniformBuffer)
        {
            wgpu::BindGroupLayoutEntry entry{};            
            entry.binding = static_cast<u32>(bindingIndex);
            entry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
            entry.buffer.hasDynamicOffset = false;
            entry.buffer.type = wgpu::BufferBindingType::Uniform;
            entries.push_back(entry);
        }

        if (slot.resourceType == ShaderResourceType::SampledImage2D)
        {
            {
                wgpu::BindGroupLayoutEntry entry{};
                entry.binding = static_cast<u32>(bindingIndex);
                entry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
				entry.texture.sampleType = wgpu::TextureSampleType::Float;
				entry.texture.viewDimension = wgpu::TextureViewDimension::e2D;            
                entries.push_back(entry);
                ++bindingIndex;
            }            

            {
                wgpu::BindGroupLayoutEntry entry{};
                entry.binding = static_cast<u32>(bindingIndex);
                entry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
				entry.sampler.type = wgpu::SamplerBindingType::Filtering;
                entries.push_back(entry);
            }
        }

        ++bindingIndex;
    }

	desc.entryCount = static_cast<u32>(entries.size());
	desc.entries = entries.data();

	auto bgl = wgpuDevice.CreateBindGroupLayout(&desc);

	gAllParameterBlockLayouts.push_back(bgl);

    return ParameterBlockLayoutHandle{ static_cast<u32>(gAllParameterBlockLayouts.size() - 1) };
}

void Device::DestroyParameterBlockLayout(ParameterBlockLayoutHandle layoutID)
{
    //vkAllocateDescriptorSets()
    // Implementation for destroying a parameter block layout
}

void CommandList::SetParameterBlock(u8 setIndex, ParameterBlockHandle parameterBlockID)
{
  //  auto cmdBuff = static_cast<VkCommandBuffer>(nativePtr);
    PSOMeta& psoMeta = gAllPSOMetas[currentPsoID.handle];

    if(currentRenderPassNativePtr != nullptr)
    {
        auto rpassEnc = (wgpu::RenderPassEncoder*)currentRenderPassNativePtr;
        rpassEnc->SetBindGroup(setIndex + (psoMeta.hasPushConstants ? 1 : 0),
            gAllParameterBlocks[parameterBlockID.handle],
                               0,
			nullptr);
	}
}

ParameterBlockHandle Device::CreateParameterBlock(const ParameterBlockLayoutHandle& layoutID)
{
    ParamaterBllockMeta meta;
    meta.layoutHandle = layoutID;

	gAllParameterBlockMetas.push_back(meta);
	gAllParameterBlocks.push_back(nullptr);

    return ParameterBlockHandle{ static_cast<u32>(gAllParameterBlocks.size() - 1)};
}

void Device::UpdateParameterBlock(ParameterBlockHandle parameterBlockID, const ParameterBlockDescriptor& pbDesc)
{
	ParamaterBllockMeta& meta = gAllParameterBlockMetas[parameterBlockID.handle];

    wgpu::BindGroupDescriptor desc{};

    std::vector<wgpu::BindGroupEntry> entries;
    u32 slotIndex = 0;

    for(int i= 0; i < pbDesc.resources.size(); ++i)
    {
        const auto& binding = pbDesc.resources[i];
        
		wgpu::BindGroupEntry entry{};
        for (const auto& res : pbDesc.resources)
        {
            std::visit([&](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;

                    if constexpr (std::is_same_v<T, ParameterResourceBuffer>)
                    {
						entry.binding = slotIndex;
                        entry.buffer = gAllBuffers[arg.buffer.handle];
                        entry.offset = arg.offset;
                        entry.size = arg.size;
						entries.push_back(entry);
                    }
                    else if constexpr (std::is_same_v<T, ParameterResourceTexture>)
                    {
                        entry.binding = slotIndex;
						entry.textureView = gAllTextures[arg.texture.handle].CreateView();
                        entries.push_back(entry);
                        slotIndex++;

                        wgpu::BindGroupEntry samplerEntry{};
						samplerEntry.binding = slotIndex;
						samplerEntry.sampler = wgpuDefaultLinearSampler;
						entries.push_back(samplerEntry);

                        MLOG_WARNING(u8"ParameterResourceTexture not yet implemented in UpdateParameterBlock");
                    }
                    else if constexpr (std::is_same_v<T, ParameterResourceRWImage>)
                    {
                        // TODO: Fill VkDescriptorImageInfo for storage image
                        MLOG_WARNING(u8"ParameterResourceRWImage not yet implemented in UpdateParameterBlock");
                    }
                    else if constexpr (std::is_same_v<T, ParameterResourceEmpty>)
                    {
                        // Intentionally empty slot – skip writing
                    }
                }, res);

            slotIndex++;
        }
	}

    desc.layout = gAllParameterBlockLayouts[meta.layoutHandle.handle];
    desc.entryCount = static_cast<u32>(entries.size());
    desc.entries = entries.data();
    desc.label = "Parameter Block";

    auto bg = wgpuDevice.CreateBindGroup(&desc);
	//DO i need to release old bind group? Probably not, WebGPU uses ref counting internally
    gAllParameterBlocks[parameterBlockID.handle] = bg;
}

void Device::DestroyParameterBlock(ParameterBlockHandle parameterBlockID)
{

}

TextureHandle Device::CreateTexture(const TextureDescriptor& desc)
{
    TextureHandle result;
    result.handle = static_cast<u32>(gAllTextures.size());
    wgpu::TextureDescriptor textureDesc{};
    textureDesc.size.width = desc.width;
    textureDesc.size.height = desc.height;
    textureDesc.size.depthOrArrayLayers = desc.depth;
    textureDesc.mipLevelCount = desc.mipLevels;
    textureDesc.sampleCount = 1;
    textureDesc.dimension = wgpu::TextureDimension::e2D;
    textureDesc.format = wgpu::TextureFormat::RGBA8Unorm;
    textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
    auto texture = wgpuDevice.CreateTexture(&textureDesc);
    gAllTextures.push_back(texture);
        
    if (desc.initialData != nullptr)
    {
		wgpu::TexelCopyTextureInfo texInfo{};
		wgpu::TexelCopyBufferInfo bufferInfo{};

		texInfo.aspect = wgpu::TextureAspect::All;
		texInfo.texture = texture;
		texInfo.mipLevel = 0;
		texInfo.origin = { 0, 0, 0 };

		bufferInfo.layout.offset = 0;
		bufferInfo.layout.bytesPerRow = static_cast<u32>(desc.width * 4);
		bufferInfo.layout.rowsPerImage = static_cast<u32>(desc.height);

        wgpuDevice.GetQueue().WriteTexture(
            &texInfo,
            desc.initialData,
            static_cast<u64>(desc.width * desc.height * 4),
            &bufferInfo.layout,
			&textureDesc.size);
    }

    TextureMeta meta;

	meta.textureView = texture.CreateView();
	gAllTextureMetas.push_back(meta);
	return result;
}

void Device::DestroyTexture(TextureHandle textureID)
{

}

void Device::UpdateTexture(TextureHandle textureID, const void* data, size_t size, size_t offset)
{
	//TODO: Implement texture update
}

void Device::UpdateSubregionTexture(
    TextureHandle textureID,
    size_t x,
    size_t y,
    size_t z,
    size_t width,
    size_t height,
    size_t depth,
    const void* data,
    size_t dataSize)
{
	//TODO: Implement subregion texture update
}

u64 TextureHandle::CreateImguiTextureOpaqueHandle() const
{
	return (u64)(intptr_t)gAllTextureMetas[handle].textureView.Get();
}

#endif //MERCURY_LL_GRAPHICS_WEBGPU#endif //MERCURY_LL_GRAPHICS_WEBGPU#endif //MERCURY_LL_GRAPHICS_WEBGPU#endif //MERCURY_LL_GRAPHICS_WEBGPU