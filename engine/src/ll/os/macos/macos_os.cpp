#include "ll/m_os.h"
#ifdef MERCURY_LL_OS_MACOS
namespace mercury
{
    namespace ll
    {
        OSInfo getOSInfo() { return OSInfo{OSType::MacOS}; }

        bool initializeOS() { return true; }

        void shutdownOS() { return; }
    } // namespace ll
} // namespace mercury
#endif