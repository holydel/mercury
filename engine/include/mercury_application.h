#pragma once

namespace mercury {
class Application {
public:
  Application();
  virtual ~Application();

  virtual void Initialize(){};
  virtual void Tick(){};
  virtual void Shutdown(){};

  virtual bool IsRunning() { return true; }
  virtual bool IsFocused() { return true; }
};
} // namespace mercury