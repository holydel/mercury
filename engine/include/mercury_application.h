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
      u8 headlessMode : 1 = false;
      u8 resizable : 1 = false;
      u8 fullscreen : 1 = false;
      u8 maximized : 1 = false;
      u8 borderless : 1 = false;
      u8 alwaysOnTop : 1 = false;
    } window;
    struct Graphics
    {
      u8 explicitAdapterIndex = 255; //if 255 - no explicit adapter index is set

      enum class AdapterTypePreference
      {
        HighPerformance,
        LowPower,
        Any
      } adapterPreference = AdapterTypePreference::Any;

      u8 enableValidationLayers : 1 = true; // Ignored in retail builds
    } graphics;

#ifdef MERCURY_LL_GRAPHICS_VULKAN
    struct VkConfig
    {

    } vkConfig;
#endif
    const c8 *appName = u8"Mercury Application";

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

    virtual bool IsRunning() { return true; }

    const Config &GetConfig() const { return config; }
    static Application *GetCurrentApplication();

    virtual void OnClose() {};
  };
} // namespace mercury