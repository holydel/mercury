#include "mercury.h"
#include <iostream>

class TestBedApplication : public mercury::Application
{
private:
  mercury::u32 frameCount = 0;
  mercury::f32 deltaTime = 0.0f;
  bool isRunning = true;

public:
  TestBedApplication()
  {
    std::cout << "TestBedApplication: Constructor called" << std::endl;
  }

  ~TestBedApplication()
  {
    std::cout << "TestBedApplication: Destructor called" << std::endl;
  }

  void Initialize() override
  {
    std::cout << "TestBedApplication: Initializing..." << std::endl;
    std::cout << "Mercury Engine Test Bed" << std::endl;
    std::cout << "=======================" << std::endl;
    std::cout << "Platform: ";

    std::cout << "C++ Standard: " << __cplusplus << std::endl;
    std::cout << "Initialization complete!" << std::endl;

    mercury::utils::debug::output_debug_string_to_ide(
        u8"Hello, World! Test String ⮐⾿ⰾ⿵⌟␭ⶫ⿐⺴⫃⛑₉▧⓪⒖");
  }

  void Tick() override
  {
    frameCount++;

    // Simulate some basic engine functionality
    if (frameCount % 60 == 0)
    {
      // std::cout << "Frame: " << frameCount << " | Delta Time: " << deltaTime
      //           << "s" << std::endl;
    }

    // Simulate frame time (16.67ms for 60 FPS)
    deltaTime = 1.0f / 60.0f;

    // Simple exit condition (run for 300 frames = 5 seconds at 60 FPS)
    if (frameCount >= 300)
    {
      isRunning = false;
    }
  }

  void Shutdown() override
  {
    std::cout << "TestBedApplication: Shutting down..." << std::endl;
    std::cout << "Total frames processed: " << frameCount << std::endl;
    std::cout << "Average FPS: " << (frameCount / 5.0f) << std::endl;
    std::cout << "Shutdown complete!" << std::endl;
  }
};

static TestBedApplication myApp;