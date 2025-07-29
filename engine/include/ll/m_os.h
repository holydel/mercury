#include "../mercury_api.h"
#include <string>

namespace mercury {
namespace ll {

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
  OS();
  ~OS();

  void Initialize();
  void Shutdown();

  const OSInfo &getOSInfo();
};

extern OS *gOS;
} // namespace ll
} // namespace mercury