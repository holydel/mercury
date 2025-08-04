#pragma once

#include "../mercury_api.h"
#include <string>

namespace mercury {
namespace ll {
namespace os {
enum class OSType : u8 {
  Windows,
  Linux,
  MacOS,
  Emscripten,
  Android,
};

enum class OSArchitecture : u8 {
  x64,
  ARM64,
};

struct OSInfo {
  OSType type;
  OSArchitecture architecture;
};

class OS {
public:
  OS() = default;
  ~OS() = default;

  void Initialize();
  void Shutdown();

  const OSInfo &GetInfo();

  bool IsFocused();

  void Sleep(u32 milliseconds);

  /// @brief Some platforms (android) may destroy the native window handle during runtime.
  /// This function should return the current native window handle.
  /// @return 
  void* GetCurrentNativeWindowHandle();
};

extern OS *gOS;
} // namespace os
} // namespace ll
} // namespace mercury