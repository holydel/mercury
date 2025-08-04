#pragma once
#include "mercury_api.h"

#if defined(MERCURY_LL_GRAPHICS_WEBGPU)
#include <webgpu/webgpu_cpp.h>

namespace wgpu_utils
{
    const char *GetBackendTypeString(wgpu::BackendType backendType);
    const char *GetAdapterTypeString(wgpu::AdapterType adapterType);
    const char *GetTextureFormatString(wgpu::TextureFormat format);
    const char *GetPresentModeString(wgpu::PresentMode presentMode);
    const char *GetCompositeAlphaModeString(wgpu::CompositeAlphaMode alphaMode);
}

#endif