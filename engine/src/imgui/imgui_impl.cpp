#include "imgui_impl.h"

#ifdef MERCURY_LL_OS_WIN32
#include <backends/imgui_impl_win32.cpp>
#endif

#ifdef MERCURY_LL_OS_ANDROID
#include <backends/imgui_impl_android.cpp>
#endif

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include <backends/imgui_impl_vulkan.cpp>
#endif

#ifdef MERCURY_LL_GRAPHICS_D3D12
#include <backends/imgui_impl_dx12.cpp>
#endif

#if defined(ALLOW_XCB_SURFACE) || defined(ALLOW_XLIB_SURFACE)
#include <backends/imgui_impl_x11.cpp>
#endif

#if defined(ALLOW_WAYLAND_SURFACE)
#include <backends/imgui_impl_wayland.cpp>
#endif