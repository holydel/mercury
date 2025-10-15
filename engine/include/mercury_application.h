#pragma once
#include "mercury_api.h"
#include "ll/graphics.h"

namespace mercury
{

  struct Config
  {
    struct Logger
    {
      u8 logToConsole : 1;
      u8 logToIDE : 1;
      u8 logToFile : 1;

      Logger()
      {
          logToConsole = true;
          logToIDE = true;
          logToFile = true;
      }
    } logger;

    struct Window
    {
      u16 width;
      u16 height;

      // flags
      bool headlessMode : 1;
      bool resizable : 1;
      bool fullscreen : 1;
      bool maximized : 1;
      bool borderless : 1;
      bool alwaysOnTop : 1;

      Window()
      {
          width = 800;
          height = 600;

          headlessMode = false;
          resizable = false;
          fullscreen = false;
          maximized = false;
          borderless = false;
          alwaysOnTop = false;
      }
    } window;

    struct EngineConfig
    {
      bool EnableIMGUI : 1;
      bool EnableSound : 1;
      bool EnableBulletPhysics : 1;
      bool EnableXR : 1;
      bool EnableXRMirroring : 1;

      EngineConfig()
      {
          EnableIMGUI = false;
          EnableSound = false;
          EnableBulletPhysics = false;
          EnableXR = false;
          EnableXRMirroring = false;
      }
    };

    struct D3D12Config
    {
      bool useWorkGraphs : 1;

      D3D12Config()
      {
		  useWorkGraphs = false;
      }
    };

    struct VKConfig
    {
      bool useDynamicRendering : 1;

      VKConfig()
      {
          useDynamicRendering = false;
	  }
    };

    struct XRConfig
    {
      bool useDebugLayers : 1;

      XRConfig()
      {
          useDebugLayers = true;
      }
    };

    struct InputConfig
    {
      u16 checkGamepadIntervalMS = 250; // for one gamepad per tick
    };

    struct ImguiConfig
    {
      bool enable : 1;
      bool enableDocking : 1;
	  bool enableViewports : 1;

      ImguiConfig()
      {
          enable = true;
		  enableDocking = false;
          enableViewports = false; 
      }
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

      u8 enableValidationLayers : 1; // Ignored in retail builds

      bool enableRaytracing : 1;
      bool enableBarycentricFS : 1;
      bool enableMeshShader : 1;
      bool enableSamplerFeedback : 1;
      bool enableVariableRateShading : 1;
      bool enableGeometryShader : 1;
      bool enableTessellation : 1;
      bool enableSamplerYCbCr : 1;
      bool enablePartialResidencyTextures : 1;
      bool enablePartialResidencyBuffers : 1;

      Graphics()
      {
          enableValidationLayers = true;

          enableRaytracing = false;
          enableBarycentricFS = false;
          enableMeshShader = false;
          enableSamplerFeedback = false;
          enableVariableRateShading = false;
          enableGeometryShader = false;
          enableTessellation = false;
          enableSamplerYCbCr = false;
          enablePartialResidencyTextures = false;
          enablePartialResidencyBuffers = false;
      }
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

      bool useAlpha : 1;
      bool tripleBuffering : 1; // for VR - use double buffering

      SwapchainConfig()
      {
		  useAlpha = false;
		  tripleBuffering = true;
      }
    } swapchain;

    const c8 *appName = MSTR("Mercury Application");
    const c8 *appID = MSTR("com.company-name.test-mercury-app-name");
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
	virtual void OnFinalPass(mercury::ll::graphics::CommandList &finalCL) {};
    virtual bool IsRunning() { return true; }

    Config &GetConfig() { return config; }
    static Application *GetCurrentApplication();

    virtual void OnClose() {};
  };
} // namespace mercury