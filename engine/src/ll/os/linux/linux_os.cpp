#include "ll/m_os.h"
#ifdef MERCURY_LL_OS_LINUX

#include "application.h"
#include <iostream>

namespace mercury
{
  namespace ll
  {
    OSInfo getOSInfo() { return OSInfo{OSType::Linux}; }

    bool initializeOS() { return true; }

    void shutdownOS() { return; }
  } // namespace ll
} // namespace mercury

int main(int argc, char **argv)
{
  std::cout << "LINUX start console" << std::endl;

  RunCurrentApplication();

  return 0;
}
#endif
