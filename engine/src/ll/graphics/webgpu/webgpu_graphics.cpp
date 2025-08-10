#include "ll/graphics.h"
#include "ll/os.h"
#include "mercury_application.h"

using namespace mercury;
using namespace mercury::ll::graphics;

#if defined(MERCURY_LL_GRAPHICS_WEBGPU)

#include "mercury_log.h"
#include <webgpu/webgpu_cpp.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include "webgpu_utils.h"
#include <emscripten/fetch.h>
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

//Temporary global variables for pipeline and shaders
wgpu::RenderPipeline gTestTrinaglePipeline;
wgpu::ShaderModule gTestTriangleVertexShader;
wgpu::ShaderModule gTestTriangleFragmentShader;
wgpu::Buffer gUniformBuffer;
wgpu::BindGroup gBindGroup;


// Point cloud rendering variables
wgpu::RenderPipeline gPointCloudPipeline;
wgpu::ShaderModule gPointCloudVertexShader;
wgpu::ShaderModule gPointCloudFragmentShader;
wgpu::Buffer gPointCloudUniformBuffer;
wgpu::BindGroup gPointCloudBindGroup;
wgpu::Buffer gPointCloudVertexBuffer;
uint32_t gPointCloudPointCount = 0;

int gInitialCanvasWidth = 0;
int gInitialCanvasHeight = 0;

// Function to get canvas surface
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

void Instance::Initialize()
{
    MLOG_DEBUG(u8"Initialize Graphics System (WEBGPU)");

    wgpu::InstanceDescriptor instanceDesc = {};
    instanceDesc.nextInChain = nullptr;
    instanceDesc.capabilities.nextInChain = nullptr;
    instanceDesc.capabilities.timedWaitAnyEnable = true;
    instanceDesc.capabilities.timedWaitAnyMaxCount = 64;
    wgpuInstance = wgpu::CreateInstance(&instanceDesc);
    wgpuSurface = GetCanvasSurface();
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

u8 Instance::GetAdapterCount()
{
    return 1;
}

void *Instance::GetNativeHandle()
{
    return wgpuInstance.Get();
}

Adapter *Instance::AcquireAdapter(const AdapterSelectorInfo &selector_info)
{
    if (gAdapter == nullptr)
        gAdapter = new Adapter();

    return gAdapter;
}

Device *Adapter::CreateDevice()
{
    if (gDevice == nullptr)
        gDevice = new Device();

    return gDevice;
}

void CreateTestTriangleShaders() {
    // Corrected Vertex shader source for WGSL with rotation uniform
    const char *vertexShaderSource = R"(
        struct Uniforms {
            angle: f32,
        }

        @group(0) @binding(0) var<uniform> uniforms: Uniforms;

        struct VSOutput {
            @builtin(position) position: vec4<f32>,
            @location(0) color: vec4<f32>,
        }

        @vertex
        fn main(@builtin(vertex_index) vertexID : u32) -> VSOutput {
            var positions = array<vec2<f32>, 3>(
                vec2<f32>(0.0, 0.5),
                vec2<f32>(-0.5, -0.5),
                vec2<f32>(0.5, -0.5)
            );

            var colors = array<vec4<f32>, 3>(
                vec4<f32>(1.0, 0.0, 0.0, 1.0), // Red
                vec4<f32>(0.0, 1.0, 0.0, 1.0), // Green
                vec4<f32>(0.0, 0.0, 1.0, 1.0)  // Blue
            );

            // Apply rotation
            let cosAngle = cos(uniforms.angle);
            let sinAngle = sin(uniforms.angle);
            let pos = positions[vertexID];
            let rotatedPos = vec2<f32>(
                pos.x * cosAngle - pos.y * sinAngle,
                pos.x * sinAngle + pos.y * cosAngle
            );

            var output : VSOutput;
            output.position = vec4<f32>(rotatedPos, 0.0, 1.0);
            output.color = colors[vertexID];
            return output;
        }
    )";

    // Corrected Fragment shader source for WGSL
    const char *fragmentShaderSource = R"(
        @fragment
        fn main(@location(0) color : vec4<f32>) -> @location(0) vec4<f32> {
            return color;
        }
    )";

    wgpu::ShaderSourceWGSL vertexShaderSrc;
    vertexShaderSrc.code = wgpu::StringView(vertexShaderSource);
    vertexShaderSrc.nextInChain = nullptr;

    wgpu::ShaderSourceWGSL fragmentShaderSrc;
    fragmentShaderSrc.code = wgpu::StringView(fragmentShaderSource);
    fragmentShaderSrc.nextInChain = nullptr;

    wgpu::ShaderModuleDescriptor vertexShaderDesc = {};
    vertexShaderDesc.nextInChain = &vertexShaderSrc;

    wgpu::ShaderModuleDescriptor fragmentShaderDesc = {};
    fragmentShaderDesc.nextInChain = &fragmentShaderSrc;

    gTestTriangleVertexShader = wgpuDevice.CreateShaderModule(&vertexShaderDesc);
    gTestTriangleFragmentShader = wgpuDevice.CreateShaderModule(&fragmentShaderDesc);
}

void CreateUniformBuffer() {
    // Create uniform buffer
    wgpu::BufferDescriptor bufferDesc = {};
    bufferDesc.size = sizeof(float); // Size for one float (angle)
    bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    bufferDesc.mappedAtCreation = false;
    gUniformBuffer = wgpuDevice.CreateBuffer(&bufferDesc);
}

void CreatePointCloudShaders() {
    // Point cloud vertex shader with MVP matrix
    const char *vertexShaderSource = R"(
        struct Uniforms {
            mvpMatrix: mat4x4<f32>,
        }

        @group(0) @binding(0) var<uniform> uniforms: Uniforms;

        struct VertexInput {
            @location(0) position: vec3<f32>,
            @location(1) color: vec4<f32>,
        }

        struct VSOutput {
            @builtin(position) position: vec4<f32>,
            @location(0) color: vec4<f32>,
        }

        @vertex
        fn main(input: VertexInput) -> VSOutput {
            var output: VSOutput;
            output.position = uniforms.mvpMatrix * vec4<f32>(input.position, 1.0);
            output.color = input.color;
            return output;
        }
    )";

    // Point cloud fragment shader
    const char *fragmentShaderSource = R"(
        @fragment
        fn main(@location(0) color: vec4<f32>) -> @location(0) vec4<f32> {
            return color;
        }
    )";

    wgpu::ShaderSourceWGSL vertexShaderSrc;
    vertexShaderSrc.code = wgpu::StringView(vertexShaderSource);
    vertexShaderSrc.nextInChain = nullptr;

    wgpu::ShaderSourceWGSL fragmentShaderSrc;
    fragmentShaderSrc.code = wgpu::StringView(fragmentShaderSource);
    fragmentShaderSrc.nextInChain = nullptr;

    wgpu::ShaderModuleDescriptor vertexShaderDesc = {};
    vertexShaderDesc.nextInChain = &vertexShaderSrc;

    wgpu::ShaderModuleDescriptor fragmentShaderDesc = {};
    fragmentShaderDesc.nextInChain = &fragmentShaderSrc;

    gPointCloudVertexShader = wgpuDevice.CreateShaderModule(&vertexShaderDesc);
    gPointCloudFragmentShader = wgpuDevice.CreateShaderModule(&fragmentShaderDesc);
}

void CreatePointCloudUniformBuffer() {
    // Create uniform buffer for MVP matrix (16 floats = 64 bytes)
    wgpu::BufferDescriptor bufferDesc = {};
    bufferDesc.size = 16 * sizeof(float); // Size for 4x4 matrix
    bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    bufferDesc.mappedAtCreation = false;
    gPointCloudUniformBuffer = wgpuDevice.CreateBuffer(&bufferDesc);
}

void CreatePipelineStateObject()
{
    // Create bind group layout for uniform buffer
    wgpu::BindGroupLayoutEntry bindGroupLayoutEntry = {};
    bindGroupLayoutEntry.binding = 0;
    bindGroupLayoutEntry.visibility = wgpu::ShaderStage::Vertex;
    bindGroupLayoutEntry.buffer.type = wgpu::BufferBindingType::Uniform;
    bindGroupLayoutEntry.buffer.minBindingSize = sizeof(float);

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {};
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &bindGroupLayoutEntry;
    wgpu::BindGroupLayout bindGroupLayout = wgpuDevice.CreateBindGroupLayout(&bindGroupLayoutDesc);

    // Create pipeline layout with bind group layout
    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc = {};
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout = wgpuDevice.CreatePipelineLayout(&pipelineLayoutDesc);

    // Create bind group
    wgpu::BindGroupEntry bindGroupEntry = {};
    bindGroupEntry.binding = 0;
    bindGroupEntry.buffer = gUniformBuffer;
    bindGroupEntry.size = sizeof(float);

    wgpu::BindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &bindGroupEntry;
    gBindGroup = wgpuDevice.CreateBindGroup(&bindGroupDesc);

    // Create fragment state
    wgpu::FragmentState fragmentState = {};
    fragmentState.module = gTestTriangleFragmentShader;
    fragmentState.entryPoint = "main";
    fragmentState.targetCount = 1;
    wgpu::ColorTargetState colorTarget = {};
    colorTarget.format = wgpu::TextureFormat::BGRA8Unorm;
    fragmentState.targets = &colorTarget;

    // Create render pipeline
    wgpu::RenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.layout = pipelineLayout;

    pipelineDesc.vertex.module = gTestTriangleVertexShader;
    pipelineDesc.vertex.entryPoint = "main";

    pipelineDesc.fragment = &fragmentState;

    gTestTrinaglePipeline = wgpuDevice.CreateRenderPipeline(&pipelineDesc);

    MLOG_DEBUG(u8"Pipeline State Object created successfully");
}

void CreatePointCloudPSO()
{
    // Create bind group layout for uniform buffer (MVP matrix)
    wgpu::BindGroupLayoutEntry bindGroupLayoutEntry = {};
    bindGroupLayoutEntry.binding = 0;
    bindGroupLayoutEntry.visibility = wgpu::ShaderStage::Vertex;
    bindGroupLayoutEntry.buffer.type = wgpu::BufferBindingType::Uniform;
    bindGroupLayoutEntry.buffer.minBindingSize = 16 * sizeof(float); // 4x4 matrix

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc = {};
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &bindGroupLayoutEntry;
    wgpu::BindGroupLayout bindGroupLayout = wgpuDevice.CreateBindGroupLayout(&bindGroupLayoutDesc);

    // Create pipeline layout with bind group layout
    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc = {};
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;
    wgpu::PipelineLayout pipelineLayout = wgpuDevice.CreatePipelineLayout(&pipelineLayoutDesc);

    // Create bind group for point cloud
    wgpu::BindGroupEntry bindGroupEntry = {};
    bindGroupEntry.binding = 0;
    bindGroupEntry.buffer = gPointCloudUniformBuffer;
    bindGroupEntry.size = 16 * sizeof(float);

    wgpu::BindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &bindGroupEntry;
    gPointCloudBindGroup = wgpuDevice.CreateBindGroup(&bindGroupDesc);

    // Define vertex attributes for point cloud
    wgpu::VertexAttribute positionAttribute = {};
    positionAttribute.format = wgpu::VertexFormat::Float32x3;
    positionAttribute.offset = 0;
    positionAttribute.shaderLocation = 0;

    wgpu::VertexAttribute colorAttribute = {};
    colorAttribute.format = wgpu::VertexFormat::Float32x4;
    colorAttribute.offset = 3 * sizeof(float);
    colorAttribute.shaderLocation = 1;

    wgpu::VertexAttribute attributes[] = {positionAttribute, colorAttribute};

    wgpu::VertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.arrayStride = 7 * sizeof(float); // 3 position + 4 color
    vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;
    vertexBufferLayout.attributeCount = 2;
    vertexBufferLayout.attributes = attributes;

    // Create fragment state
    wgpu::FragmentState fragmentState = {};
    fragmentState.module = gPointCloudFragmentShader;
    fragmentState.entryPoint = "main";
    fragmentState.targetCount = 1;
    wgpu::ColorTargetState colorTarget = {};
    colorTarget.format = wgpu::TextureFormat::BGRA8Unorm;
    fragmentState.targets = &colorTarget;

    // Create render pipeline for point cloud
    wgpu::RenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.layout = pipelineLayout;

    pipelineDesc.vertex.module = gPointCloudVertexShader;
    pipelineDesc.vertex.entryPoint = "main";
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;

    pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::PointList;
    pipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
    pipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;
    pipelineDesc.primitive.cullMode = wgpu::CullMode::None;

    pipelineDesc.fragment = &fragmentState;

    gPointCloudPipeline = wgpuDevice.CreateRenderPipeline(&pipelineDesc);

    MLOG_DEBUG(u8"Point Cloud Pipeline State Object created successfully");
}

void DrawTestTriangle(wgpu::RenderPassEncoder rpEncoder)
{
    rpEncoder.SetPipeline(gTestTrinaglePipeline);
    rpEncoder.SetBindGroup(0, gBindGroup);
    rpEncoder.Draw(3, 1, 0, 0);
}

void DrawPointCloud(wgpu::RenderPassEncoder rpEncoder, wgpu::Buffer vertexBuffer, uint32_t pointCount)
{
    rpEncoder.SetPipeline(gPointCloudPipeline);
    rpEncoder.SetBindGroup(0, gPointCloudBindGroup);
    rpEncoder.SetVertexBuffer(0, vertexBuffer);
    rpEncoder.Draw(pointCount, 1, 0, 0);
}

void UpdatePointCloudMVPMatrix(const float* mvpMatrix)
{
    // Update the point cloud uniform buffer with the MVP matrix
    wgpuDevice.GetQueue().WriteBuffer(gPointCloudUniformBuffer, 0, mvpMatrix, 16 * sizeof(float));
}

wgpu::Buffer CreatePointCloudVertexBuffer(const void* pointData, size_t dataSize)
{
    wgpu::BufferDescriptor bufferDesc = {};
    bufferDesc.size = dataSize;
    bufferDesc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    bufferDesc.mappedAtCreation = false;
    
    wgpu::Buffer buffer = wgpuDevice.CreateBuffer(&bufferDesc);
    wgpuDevice.GetQueue().WriteBuffer(buffer, 0, pointData, dataSize);
    
    return buffer;
}

void LoadPointCloudFromURL(const char* url) {

    MLOG_DEBUG(u8"Starting to fetch point cloud from: %s", url);
    
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    
    attr.onsuccess = [](emscripten_fetch_t *fetch) {
        MLOG_DEBUG(u8"Successfully fetched %zu bytes from URL", fetch->numBytes);


        gPointCloudVertexBuffer = CreatePointCloudVertexBuffer(fetch->data, fetch->numBytes);
        gPointCloudPointCount = fetch->numBytes / (sizeof(float) * 7); // Assuming PointCloudVertex is defined correctly

        emscripten_fetch_close(fetch);
    };
    
    attr.onerror = [](emscripten_fetch_t *fetch) {
        MLOG_ERROR(u8"Failed to fetch point cloud from URL (HTTP %d)", fetch->status);
        
        emscripten_fetch_close(fetch);
    };
    
    emscripten_fetch(&attr, url);
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
        adapterOptions.powerPreference = graphicsConfig.preferHighPerformance ? wgpu::PowerPreference::HighPerformance : wgpu::PowerPreference::LowPower;
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

    CreateTestTriangleShaders();
    CreateUniformBuffer();
    CreatePipelineStateObject();
    
    CreatePointCloudShaders();
    CreatePointCloudUniformBuffer();
    CreatePointCloudPSO();
    //InitializePointCloudData();

    LoadPointCloudFromURL("pointcache.bin");
}

void Device::Shutdown()
{
    MLOG_DEBUG(u8"Shutdown Device (WEBGPU)");
}

void Device::Tick()
{
}

void *Device::GetNativeHandle()
{
    return wgpuDevice.Get();
}

void Device::InitializeSwapchain(void *native_window_handle)
{
    assert(native_window_handle != nullptr && "Native window handle cannot be null");
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

void Swapchain::AcquireNextImage()
{
    wgpuSurface.GetCurrentTexture(&wgpuCurrentSwapchainTexture);
}

void ClearTextureWithColor(float r, float g, float b, float a)
{
    static float rotationAngle = 0.0f;
    rotationAngle += 0.02f; // Increment rotation angle

    // Update uniform buffer with current rotation angle
    wgpuDevice.GetQueue().WriteBuffer(gUniformBuffer, 0, &rotationAngle, sizeof(float));

    // Create a rotating MVP matrix for point cloud around Y-axis
    static float pointCloudRotationY = 0.0f;
    pointCloudRotationY += 0.01f; // Rotate slower than triangle
    
    // Add floating Z movement
    static float zFloatTime = 0.0f;
    zFloatTime += 0.005f; // Very slow floating
    float zDistance = -3.0f + sin(zFloatTime) * 0.5f; // Float between -3.5 and -2.5
    
    // Create perspective projection matrix using GLM
    float fov = glm::radians(45.0f); // 45 degrees in radians
    float aspect = 1024.0f / 576.0f; // Use actual surface dimensions
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    
    glm::mat4 projectionMatrix = glm::perspectiveRH_ZO(fov, aspect, nearPlane, farPlane);
    
    // Create view matrix using GLM (look at camera positioned back on Z-axis)
    glm::vec3 eye(0.0f, 0.0f, -zDistance);  // Camera position (note: negated zDistance)
    glm::vec3 center(0.0f, 0.0f, 0.0f);     // Look at origin
    glm::vec3 up(0.0f, 1.0f, 0.0f);         // Up vector
    glm::mat4 viewMatrix = glm::lookAt(eye, center, up);
    
    // Create model matrix using GLM
    float modelScale = 0.005f;
    glm::mat4 modelMatrix = glm::mat4(1.0f); // Identity matrix
    modelMatrix = glm::scale(modelMatrix, glm::vec3(modelScale)); // Apply scale
    modelMatrix = glm::rotate(modelMatrix, pointCloudRotationY, glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate around Y-axis
    
    // Calculate MVP matrix using GLM
    glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;
    
    // Update point cloud MVP matrix (GLM matrices are column-major, which WebGPU expects)
    UpdatePointCloudMVPMatrix(glm::value_ptr(mvpMatrix));

    // Debug: Log some matrix info periodically
    static int debugCounter = 0;
    if (debugCounter % 300 == 0) { // Every ~5 seconds at 60fps
        MLOG_DEBUG(u8"Cube Debug - FOV: %.1f deg, Aspect: %.3f, Z-distance: %.2f, Rotation: %.2f", 
                   glm::degrees(fov), aspect, zDistance, glm::degrees(pointCloudRotationY));
    }
    debugCounter++;

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

    //DrawTestTriangle(renderPass);
    if(gPointCloudPointCount > 0)
        DrawPointCloud(renderPass, gPointCloudVertexBuffer, gPointCloudPointCount); // Draw point cloud with vertex buffer and count
    renderPass.End();

    // Finish encoding and submit the commands
    wgpu::CommandBuffer commandBuffer = commandEncoder.Finish();
    wgpuDevice.GetQueue().Submit(1, &commandBuffer);
}

void Swapchain::Present()
{
    static float a = 0.0f;
    a += 0.01f;

    ClearTextureWithColor(sin(a) * 0.5f + 0.5f, cos(a) * 0.5f + 0.5f, 0.5f, 1.0f);
  
    #ifndef MERCURY_LL_OS_EMSCRIPTEN
    wgpuSurface.Present();
    #endif
}




#endif //MERCURY_LL_GRAPHICS_WEBGPU