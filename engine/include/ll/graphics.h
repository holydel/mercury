#pragma once

#include "../mercury_api.h"
#include "mercury_application.h"
#include <string>
#include <glm/glm.hpp>
namespace mercury {
namespace ll {
namespace graphics {
enum class Format {
  // Common formats
  RGBA8_UNORM,
  RGBA8_UNORM_SRGB,
  BGRA8_UNORM,
  RGBA16_FLOAT,
  RGBA32_FLOAT,
  R8_UNORM,
  RG8_UNORM,
  R16_FLOAT,
  RG16_FLOAT,
  R32_FLOAT,
  RG32_FLOAT,
  DEPTH24_UNORM_STENCIL8,
  DEPTH32_FLOAT,
  DEPTH16_UNORM,
  BC1_UNORM,
  BC1_UNORM_SRGB,
  BC2_UNORM,
  BC2_UNORM_SRGB,
  BC3_UNORM,
  BC3_UNORM_SRGB,
  BC4_UNORM,
  BC4_SNORM,
  BC5_UNORM,
  BC5_SNORM,
  BC6H_UFLOAT,
  BC6H_SFLOAT,
  BC7_UNORM,
  BC7_UNORM_SRGB,
  ASTC_4X4_UNORM,
  ASTC_4X4_UNORM_SRGB,
  ETC2_RGB_UNORM,
  ETC2_RGB_UNORM_SRGB,
  R8_SNORM,
  R8_UINT,
  R8_SINT,
  RG8_SNORM,
  RG8_UINT,
  RG8_SINT,
  RGBA8_SNORM,
  RGBA8_UINT,
  RGBA8_SINT,
  R16_UNORM,
  R16_SNORM,
  R16_UINT,
  R16_SINT,
  RG16_UNORM,
  RG16_SNORM,
  RG16_UINT,
  RG16_SINT,
  RGBA16_UNORM,
  RGBA16_SNORM,
  RGBA16_UINT,
  RGBA16_SINT,
  R32_UINT,
  R32_SINT,
  RG32_UINT,
  RG32_SINT,
  RGBA32_UINT,
  RGBA32_SINT,
  R64_FLOAT,
  RG64_FLOAT,
  RGBA64_FLOAT,
  R10G10B10A2_UNORM,
  R10G10B10A2_UINT,
  RG11B10_FLOAT,
  R9G9B9E5_UFLOAT,
  B5G6R5_UNORM,
  B5G5R5A1_UNORM,
  R4G4_UNORM,
  R4G4B4A4_UNORM,
  B4G4R4A4_UNORM,
  R5G5B5A1_UNORM,
  A1R5G5B5_UNORM,
  R10X6G10X6B10X6A10X6_UNORM,
  R12X4G12X4B12X4A12X4_UNORM,
  STENCIL8_UINT,
  DEPTH32_FLOAT_STENCIL8,
  DEPTH16_UNORM_STENCIL8,

  // DirectX-specific formats
  D3D_AYUV,
  D3D_Y410,
  D3D_Y416,
  D3D_NV12,
  D3D_P010,
  D3D_P016,
  D3D_420_OPAQUE,
  D3D_YUY2,
  D3D_Y210,
  D3D_Y216,
  D3D_NV11,
  D3D_AI44,
  D3D_IA44,
  D3D_P8,
  D3D_A8P8,
  D3D_P208,
  D3D_V208,
  D3D_V408,
  D3D_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE,
  D3D_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE,

  // Vulkan-specific formats
  VK_R8_BOOL_ARM,
  VK_R16G16_SFIXED5_NV,
  VK_G8B8G8R8_UNORM,
  VK_G8_B8_R8_3PLANE_420_UNORM,
  VK_G8_B8R8_2PLANE_420_UNORM,
  VK_G8_B8_R8_3PLANE_422_UNORM,
  VK_G8_B8R8_2PLANE_422_UNORM,
  VK_G8_B8_R8_3PLANE_444_UNORM,
  VK_R10X6_UNORM_PACK16,
  VK_R10X6G10X6_UNORM_2PACK16,
  VK_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
  VK_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
  VK_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
  VK_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
  VK_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
  VK_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
  VK_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
  VK_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
  VK_R12X4_UNORM_PACK16,
  VK_R12X4G12X4_UNORM_2PACK16,
  VK_R12X4G12X4B12X4A12X4_UNORM_4PACK16,
  VK_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
  VK_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
  VK_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
  VK_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
  VK_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
  VK_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
  VK_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
  VK_G16B16G16R16_422_UNORM,
  VK_B16G16R16G16_422_UNORM,
  VK_G16_B16_R16_3PLANE_420_UNORM,
  VK_G16_B16R16_2PLANE_420_UNORM,
  VK_G16_B16_R16_3PLANE_422_UNORM,
  VK_G16_B16R16_2PLANE_422_UNORM,
  VK_G16_B16_R16_3PLANE_444_UNORM,

  // Metal-specific formats
  MTL_A8_UNORM,
  MTL_R8_UNORM_SRGB,
  MTL_RG8_UNORM_SRGB,
  MTL_BGRA8_UNORM_SRGB,
  MTL_RGBA16_UNORM_SRGB,
  MTL_RGBA32_FLOAT_SRGB,
  MTL_DEPTH24_UNORM_STENCIL8,
  MTL_DEPTH32_FLOAT_STENCIL8,
  MTL_BGR10_XR_SRGB,
  // WebGPU-specific formats
  WGPU_BGRA8_UNORM_SRGB,
  WGPU_DEPTH24PLUS,
  WGPU_DEPTH24PLUS_STENCIL8,
};

struct TimelineSemaphore
{
  void* nativePtr;

  void WaitUntil(mercury::u64 value, mercury::u64 timeout = 0);
  void SetDebugName(const char* utf8_name);
  void Destroy();
};

struct RenderPass
{
  void* nativePtr;

  void SetDebugName(const char* utf8_name);
  void Destroy();
};

struct CommandList
{
  void* nativePtr;  

  bool IsExecuted();
  void SetDebugName(const char* utf8_name);
  void Destroy();
};

struct CommandPool
{
  void* nativePtr;

  CommandList AllocateCommandList();
  void SetDebugName(const char* utf8_name);
  void Destroy();
  void Reset();
};

// Forward declarations
class Instance;
class Adapter;
class Device;
class Swapchain;

struct AdapterSelectorInfo {
  u8 adapter_index = 255;

  Config::Graphics::AdapterTypePreference adapter_type_preference = Config::Graphics::AdapterTypePreference::Any;
};

/// @brief Cross GAPI physical device information.
struct AdapterInfo
{
  enum class Type {
    Integrated,
    Discrete,
    Virtual,
    Software,
    Unknown
  } type = Type::Unknown;

  enum class Vendor {
    AMD,
    NVIDIA,
    Intel,
    ARM,
    Apple,
    Qualcomm,
    Imagination,
    Unknown
  } vendor = Vendor::Unknown;

  u64 device_id = 0;
  u64 vendor_id = 0;
  u32 api_version = 0; // Vulkan API version, DirectX feature level, etc.
  struct SupportedFeatures
  {
    u8 geometry_shader : 1;
    u8 tessellation_shader : 1;
    u8 barycentric_coords_in_fragment_shader : 1;
  } supported_features = {};

  std::u8string name; // Device name, e.g. "NVIDIA GeForce RTX 3080"
  std::u8string driver_version; // Driver version, e.g. "460.
  std::u8string vendor_name; // Vendor name, e.g. "NVIDIA Corporation"
};

class Instance {
public:
  Instance() = default;
  ~Instance() = default;
  void* GetNativeHandle();

  void Initialize();
  void Shutdown();

  u8 GetAdapterCount();
  
  /// @brief Get information about the adapter at the specified index. Use it for custom adapter selection.
  const AdapterInfo& GetAdapterInfo(u8 index) const;

  /// @brief Acquire an adapter based on the provided selector information.
  /// Acquire the adapter and set it as global gAdapter value.
  void AcquireAdapter(const AdapterSelectorInfo &selector_info = AdapterSelectorInfo());
  void SetDebugName(const char* utf8_name);
};

enum class QueueType : u8
{
  Graphics,
  Compute,
  Transfer,
  VideoEncode,
  VideoDecode,
  OpticalFlow,
  DedicatedSparseManagement,
  DedicatedReadback
};

class Adapter {
public:
  Adapter() = default;
  ~Adapter() = default;
  void* GetNativeHandle();

  void Initialize();
  void Shutdown();

  /// @brief Create a device for this adapter.
  /// @note This will set the global gDevice value.
  void CreateDevice();
  void SetDebugName(const char* utf8_name);
};

class Device {
public:
  Device() = default;
  ~Device() = default;
  void* GetNativeHandle();

  void Initialize();
  void Shutdown();

  void Tick();  

  /// @brief Initialize the swapchain for rendering.
  /// @param native_window_handle The handle to the native window where the swapchain will be created.
  /// gSwapchain will be created if it does not exist.
  void InitializeSwapchain(void *native_window_handle); 
  
  void ShutdownSwapchain();

  CommandPool CreateCommandPool(QueueType queue_type);
  TimelineSemaphore CreateTimelineSemaphore(mercury::u64 initial_value);
  void WaitIdle();
  void WaitQueueIdle(QueueType queue_type);
  void SetDebugName(const char* utf8_name);
};

class Swapchain {
public:
  glm::vec4 clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
  float clearDepth = 1.0f;
  u8 clearStencil = 0;

  Swapchain() = default;
  ~Swapchain() = default;
  void* GetNativeHandle();

  void Initialize(void* native_window_handle = nullptr);
  void ReInitIfNeeded();

  void Shutdown();

  /// @returns final pass command list
  CommandList AcquireNextImage();
  void Present();

  void SetFullscreen(bool fullscreen);
  u8 GetNumberOfFrames();

  void SetDebugName(const char* utf8_name);

  void Resize(u16 width, u16 height);
};

extern Device *gDevice;
extern Instance *gInstance;
extern Adapter *gAdapter;
extern Swapchain *gSwapchain;

const char* GetBackendName();
} // namespace graphics
} // namespace ll
} // namespace mercury