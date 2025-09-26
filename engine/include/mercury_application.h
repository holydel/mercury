#pragma once
#include "mercury_api.h"

namespace mercury
{

  struct Config
  {
    struct Logger
    {
      u8 logToConsole : 1 = true;
      u8 logToIDE : 1 = true;
      u8 logToFile : 1 = false;
    } logger;

    struct Window
    {
      u16 width = 800;
      u16 height = 600;

      // flags
      bool headlessMode : 1 = false;
      bool resizable : 1 = false;
      bool fullscreen : 1 = false;
      bool maximized : 1 = false;
      bool borderless : 1 = false;
      bool alwaysOnTop : 1 = false;
    } window;

    struct EngineConfig
    {
      bool EnableIMGUI : 1 = false;
      bool EnableSound : 1 = false;
      bool EnableBulletPhysics : 1 = false;
      bool EnableXR : 1 = false;
      bool EnableXRMirroring : 1 = false;
    };

    struct D3D12Config
    {
      bool useWorkGraphs = false;
    };

    struct VKConfig
    {
      bool useDynamicRendering = false;
    };

    struct XRConfig
    {
      bool useDebugLayers = true;
    };

    struct InputConfig
    {
      u16 checkGamepadIntervalMS = 250; // for one gamepad per tick
    };

    struct ImguiConfig
    {
      bool enable : 1 = true;
      bool enableDocking : 1 = false;
	  bool enableViewports : 1 = false;
    } imgui;
    struct Graphics
    {
#ifdef MERCURY_LL_GRAPHICS_VULKAN
      VKConfig vkConfig;
#endif
#ifdef MERCURY_LL_GRAPHICS_D3D12
      D3D12Config d3d12Config;
#endif

      u8 explicitAdapterIndex = 255; // if 255 - no explicit adapter index is set

      enum class AdapterTypePreference
      {
        HighPerformance,
        LowPower,
        Any
      } adapterPreference = AdapterTypePreference::Any;

      u8 enableValidationLayers : 1 = true; // Ignored in retail builds

      bool enableRaytracing : 1 = false;
      bool enableBarycentricFS : 1 = false;
      bool enableMeshShader : 1 = false;
      bool enableSamplerFeedback : 1 = false;
      bool enableVariableRateShading : 1 = false;
      bool enableGeometryShader : 1 = false;
      bool enableTessellation : 1 = false;
      bool enableSamplerYCbCr : 1 = false;
      bool enablePartialResidencyTextures : 1 = false;
      bool enablePartialResidencyBuffers : 1 = false;
    } graphics;

    struct SwapchainConfig
    {
      enum class ColorWidth : u8
      {
        Prefer16bit,
        Prefer32bit,
        PreferHDR
      };

      enum class DepthMode : u8
      {
        None,
        Depth16,
        Depth24_Stencil8,
        Depth32F,
        Depth32F_Stencil8
      };

      enum class MSAAMode : u8
      {
        None = 1,
        Samples2 = 2,
        Samples4 = 4,
        Samples8 = 8,
        Samples16 = 16
      };

      ColorWidth colorWidth = ColorWidth::Prefer32bit;
      DepthMode depthMode = DepthMode::None;
      MSAAMode msaaMode = MSAAMode::None;

      enum class VSyncMode : u8
      {
        NoVSync,
        AdaptiveVSync,
        AlwaysVSync
      };

      VSyncMode vsync = VSyncMode::AlwaysVSync;

      bool useAlpha : 1 = false;
      bool tripleBuffering : 1 = true; // for VR - use double buffering
    } swapchain;

    const c8 *appName = u8"Mercury Application";
    const c8 *appID = u8"com.company-name.test-mercury-app-name";
    struct Version
    {
      union
      {
        struct
        {
          u8 major;
          u8 minor;
          u16 patch;
        };

        u32 packed;
      };
    } appVersion = Version{{{0, 0, 1}}};
  };

  class Application
  {
  protected:
    Config config;

  public:
    Application();
    virtual ~Application();

    virtual void Configure() {};
    virtual void Initialize() {};
    virtual void Tick() {};
    virtual void Shutdown() {};
    virtual void OnImgui() {};
    virtual bool IsRunning() { return true; }

    Config &GetConfig() { return config; }
    static Application *GetCurrentApplication();

    virtual void OnClose() {};
  };
} // namespace mercury