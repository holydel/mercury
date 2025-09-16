#include "mercury_swapchain.h"
#include "mercury_log.h"

SwapchainConfig gSwapchainConfig;

void mercurySwapchainConfigure(const mercury::Config::SwapchainConfig& swapchainConfig)
{
	MLOG_DEBUG(u8"SWAPCHAIN Configure");

	gSwapchainConfig.useDisplay = false;
	gSwapchainConfig.resizable = true;
	gSwapchainConfig.needExclusiveFullscreen = true;

	gSwapchainConfig.initialAppConfig = swapchainConfig;
}