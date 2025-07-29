#pragma once

// Platform detection
#ifdef _WIN32
#define MERCURY_LL_OS_WIN32
#elif defined(__APPLE__)
#define MERCURY_LL_OS_MACOS
#elif defined(__linux__)
#define MERCURY_LL_OS_LINUX
#elif defined(__EMSCRIPTEN__)
#define MERCURY_LL_OS_EMSCRIPTEN
#elif defined(__ANDROID__)
#define MERCURY_LL_OS_ANDROID
#endif

#if defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>
#else
#if defined(MERCURY_LL_OS_WIN32) || defined(MERCURY_LL_OS_LINUX) || \
    defined(MERCURY_LL_OS_MACOS) || defined(MERCURY_LL_OS_EMSCRIPTEN)
#include <immintrin.h>
#endif
#endif

#define MERCURY_LL_GRAPHICS_NULL
// #define MERCURY_LL_GRAPHICS_VULKAN
// #define MERCURY_LL_GRAPHICS_D3D12
// #define MERCURY_LL_GRAPHICS_METAL
// #define MERCURY_LL_GRAPHICS_WEBGPU

#define MERCURY_LL_SOUND_NONE
// #define MERCURY_LL_SOUND_MINIAUDIO

namespace mercury
{
    typedef int i32;
    typedef unsigned int u32;
    typedef short i16;
    typedef unsigned short u16;
    typedef char i8;
    typedef unsigned char u8;
    typedef long long i64;
    typedef unsigned long long u64;
    // Floating point types
    typedef float f32;
    typedef double f64;

#if defined(__ARM_NEON)
    typedef float32x4_t f32x4;
    typedef int32x4_t i32x4;
    typedef uint32x4_t u32x4;
    typedef float64x2_t f64x2;
#else
    typedef __m128 f32x4;
    typedef __m128i i32x4;
    typedef __m128i u32x4;
    typedef __m128d f64x2;
#endif

} // namespace mercury