#include "imgui_impl.h"

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include <backends/imgui_impl_vulkan.cpp>
#endif

#ifdef MERCURY_LL_GRAPHICS_D3D12
#include <backends/imgui_impl_dx12.cpp>
#endif

#ifdef MERCURY_LL_GRAPHICS_WEBGPU
#include <backends/imgui_impl_wgpu.cpp>
#endif

#ifdef MERCURY_LL_GRAPHICS_METAL
#include <backends/imgui_impl_metal.mm>
#endif