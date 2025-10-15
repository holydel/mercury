#include "mercury_application.h"
#include "../../graphics.h"

struct SwapchainConfig
{
    bool useDisplay : 1;
    bool resizable : 1;
    bool needExclusiveFullscreen : 1;

    mercury::Config::SwapchainConfig initialAppConfig;

    SwapchainConfig()
    {
        useDisplay = false;
        resizable = false;
		needExclusiveFullscreen = false;
    }
};

void mercurySwapchainConfigure(const mercury::Config::SwapchainConfig& swapchainConfig);

extern SwapchainConfig gSwapchainConfig;