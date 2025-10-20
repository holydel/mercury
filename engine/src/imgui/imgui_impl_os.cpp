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
#include <backends/imgui_impl_osx.mm>
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