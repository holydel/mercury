#include "imgui_impl.h"

#ifdef MERCURY_LL_OS_WIN32
#ifndef MERCURY_UWP
#include <backends/imgui_impl_win32.cpp>
#else
#include <backends/imgui_impl_uwp.cpp>
#endif
#endif

#ifdef MERCURY_LL_OS_ANDROID
#include <backends/imgui_impl_android.cpp>
#endif

#ifdef MERCURY_LL_OS_MACOS
#ifndef MERCURY_LL_GRAPHICS_METAL
#include <backends/imgui_impl_osx.mm>
#endif
#endif

#if defined(MERCURY_LL_OS_EMSCRIPTEN)
#include <backends/imgui_impl_emscripten.cpp>
#endif

#if defined(ALLOW_XCB_SURFACE) || defined(ALLOW_XLIB_SURFACE)
#include <backends/imgui_impl_x11.cpp>
#endif

#if defined(ALLOW_WAYLAND_SURFACE)
#include <backends/imgui_impl_wayland.cpp>
#endif

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
#ifndef MERCURY_LL_OS_MACOS
#include <backends/imgui_impl_metal.mm>
#endif
#endif