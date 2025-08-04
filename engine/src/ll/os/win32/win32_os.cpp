#include "ll/os.h"
#ifdef MERCURY_LL_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "application.h"
#include <Windows.h>
#include <iostream>
#include <timeapi.h>


#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "Winmm.lib")

namespace mercury::ll::os
{
  const OSInfo &OS::GetInfo() { 
    static OSInfo info{OSType::Windows, OSArchitecture::x64};
    return info; 
  }

  void OS::Initialize() { 
    std::cout << "OS::Initialize (Emscripten)" << std::endl;
  }

  void OS::Shutdown() {
    std::cout << "OS::Shutdown (Emscripten)" << std::endl;
  }

  bool OS::IsFocused() { return true; }

  void OS::Sleep(u32 milliseconds) {
    Sleep(milliseconds);
  }

  OS* gOS = nullptr;
} 

// entry point for win32

void WinPlatformInit() {
  timeBeginPeriod(1);
  SetProcessDPIAware();

  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
  setlocale(LC_CTYPE, "en_US.UTF-8");
  setlocale(LC_COLLATE, "en_US.UTF-8");
}

int main(int argc, char **argv) {
  // Set console code page to UTF-8
  WinPlatformInit();

  std::cout << "WIN32 start console" << std::endl;

  RunCurrentApplication();

  return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  WinPlatformInit();
  std::cout << "WIN32 start WinMain" << std::endl;

  RunCurrentApplication();

  return 0;
}
#endif