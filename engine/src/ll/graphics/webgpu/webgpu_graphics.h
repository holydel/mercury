#pragma once
#include <mercury_api.h>

#ifdef MERCURY_LL_GRAPHICS_WEBGPU
#include "ll/graphics.h"

#include "webgpu_utils.h"

extern wgpu::Instance wgpuInstance;
extern wgpu::Sampler wgpuDefaultLinearSampler;
extern wgpu::Sampler wgpuDefaultNearestSampler;
extern wgpu::Sampler wgpuDefaultTrilinearSampler;
extern wgpu::Sampler wgpuDefaultAnisotropicSampler;
#endif