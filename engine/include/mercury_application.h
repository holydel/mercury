#pragma once
#include "mercury_api.h"

namespace mercury {

struct Config
{
  struct Logger
  {
    u8 logToConsole : 1= true;
    u8 logToIDE : 1= true;
    u8 logToFile : 1 = false;
  } logger;

  struct Graphics
  {
    u8 preferHighPerformance : 1 = true;
  } graphics;
};

class Application {
protected:
  Config config;
public:
  Application();
  virtual ~Application();

  virtual void Initialize(){};
  virtual void Tick(){};
  virtual void Shutdown(){};

  virtual bool IsRunning() { return true; }

  const Config &GetConfig() const { return config; }
  static Application *GetCurrentApplication();


};
} // namespace mercury