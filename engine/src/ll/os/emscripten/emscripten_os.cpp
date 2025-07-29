#include "ll/m_os.h"
#ifdef MERCURY_LL_OS_EMSCRIPTEN

#include "application.h"
#include <emscripten.h>

namespace mercury
{
  namespace ll
  {
    OSInfo getOSInfo() { return OSInfo{OSType::Emscripten}; }

    bool initializeOS() { return true; }

    void shutdownOS() { return; }
  } // namespace ll
} // namespace mercury
#endif

int main()
{
  emscripten_set_main_loop(TickCurrentApplication, 0, 1);
  return 0;
}