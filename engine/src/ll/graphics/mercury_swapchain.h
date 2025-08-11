#include "mercury_application.h"
#include "ll_graphics.h"

struct SwapchainConfig
{
    bool useDisplay : 1 = false;
    bool resizable : 1 = false;
    bool needExclusiveFullscreen : 1 = false;

    mercury::Config::SwapchainConfig initialAppConfig;
};

void mercurySwapchainConfigure(const mercury::Config::SwapchainConfig& swapchainConfig);

extern SwapchainConfig gSwapchainConfig;