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

#if defined(MERCURY_LL_OS_EMSCRIPTEN)
#include <backends/imgui_impl_emscripten.h>
#endif

#if defined(ALLOW_XCB_SURFACE) || defined(ALLOW_XLIB_SURFACE)
#include <backends/imgui_impl_x11.h>
#endif

#if defined(ALLOW_WAYLAND_SURFACE)
#include <backends/imgui_impl_wayland.h>
#endif

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include "../ll/graphics/vulkan/mercury_vulkan.h"
#define VOLK_H_
#include <backends/imgui_impl_vulkan.h>
#endif

#ifdef MERCURY_LL_GRAPHICS_D3D12
#include <backends/imgui_impl_dx12.h>
#endif

#ifdef MERCURY_LL_GRAPHICS_WEBGPU
#define IMGUI_IMPL_WEBGPU_BACKEND_DAWN
#include <backends/imgui_impl_wgpu.h>
#endif



