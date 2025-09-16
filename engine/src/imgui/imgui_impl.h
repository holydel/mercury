#pragma once
#include <mercury_api.h>

#include <imgui.h>

#ifdef MERCURY_LL_OS_WIN32
#include <Windows.h>
#include <backends/imgui_impl_win32.h>
#endif

#ifdef MERCURY_LL_OS_ANDROID
#include <backends/imgui_impl_android.h>
#endif

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include "../ll/graphics/vulkan/mercury_vulkan.h"
#define VOLK_H_
#include <backends/imgui_impl_vulkan.h>
#endif

#ifdef MERCURY_LL_GRAPHICS_D3D12
#include <backends/imgui_impl_dx12.h>
#endif
