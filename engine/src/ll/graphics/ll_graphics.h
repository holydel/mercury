#pragma once

#include "ll/graphics.h"
#include <vector>

void MercuryGraphicsInitialize();
void MercuryGraphicsShutdown();
void MercuryGraphicsTick();

extern std::vector<mercury::ll::graphics::AdapterInfo> gAllAdaptersInfo;

extern mercury::u8 SelectAdapterByHeuristic(
    const mercury::ll::graphics::AdapterSelectorInfo &selector_info
);