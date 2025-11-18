#pragma once
#include "ll/graphics.h"

#if defined(MERCURY_LL_GRAPHICS_D3D12)
#include <dxgi1_4.h>
mercury::ll::graphics::Format FromDXGIFormat(DXGI_FORMAT dxgiFormat);
DXGI_FORMAT ToDXGIFormat(mercury::ll::graphics::Format format);
#endif