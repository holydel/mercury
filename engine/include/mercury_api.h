#pragma once

#include <type_traits>
#include <concepts>

// Platform detection
#if defined(__EMSCRIPTEN__)
#define MERCURY_LL_OS_EMSCRIPTEN
#elif defined(_WIN32)
#define MERCURY_LL_OS_WIN32
#elif defined(__APPLE__)
#define MERCURY_LL_OS_MACOS
#elif defined(__linux__)
#define MERCURY_LL_OS_LINUX
#elif defined(__ANDROID__)
#define MERCURY_LL_OS_ANDROID
#endif

// SIMD support includes
#if defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
// Emscripten WebAssembly SIMD support
#include <wasm_simd128.h>
#elif defined(MERCURY_LL_OS_WIN32) || defined(MERCURY_LL_OS_LINUX) || \
    defined(MERCURY_LL_OS_MACOS)
#include <immintrin.h>
#endif

//#define MERCURY_LL_GRAPHICS_NULL
#define MERCURY_LL_GRAPHICS_VULKAN
//#define MERCURY_LL_GRAPHICS_D3D12
//  #define MERCURY_LL_GRAPHICS_METAL
//#define MERCURY_LL_GRAPHICS_WEBGPU

#define MERCURY_LL_SOUND_NONE
// #define MERCURY_LL_SOUND_MINIAUDIO

// #define MERCURY_CPU_PROFILER_AVAILABLE
// #define MERCURY_GPU_PROFILER_AVAILABLE

//Chose one
//#define MERCURY_RETAIL_BUILD
#define MERCURY_DEBUG_BUILD
//#define MERCURY_DEV_BUILD

#define MERCURY_USE_MEMORY_STAT
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

// SIMD vector types
#if defined(__ARM_NEON)
    // ARM NEON SIMD types
    typedef float32x4_t f32x4;
    typedef int32x4_t i32x4;
    typedef uint32x4_t u32x4;
    typedef float64x2_t f64x2;
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    // Emscripten WebAssembly SIMD types
    typedef v128_t f32x4;
    typedef v128_t i32x4;
    typedef v128_t u32x4;
    typedef v128_t f64x2;
#else
    // x86/x64 SSE SIMD types
    typedef __m128 f32x4;
    typedef __m128i i32x4;
    typedef __m128i u32x4;
    typedef __m128d f64x2;
#endif

    typedef char8_t c8;
    typedef char16_t c16;
    typedef char32_t c32;

    inline constexpr unsigned long long operator""_KB(unsigned long long v)
    {
        return v * 1024;
    }

    inline constexpr unsigned long long operator""_MB(unsigned long long v)
    {
        return v * 1024 * 1024;
    }

    inline constexpr unsigned long long operator""_GB(unsigned long long v)
    {
        {
            return v * 1024 * 1024 * 1024;
        }
    }

	template<std::unsigned_integral T>
	struct Handle
	{
		static constexpr T InvalidValue = (T)-1;

		T handle = InvalidValue;

		bool isValid() const
		{
			return handle != InvalidValue;
		}
		
		T Value() const
		{
			return handle;
		}
		
		void Invalidate()
		{
			handle = InvalidValue;
		}
	};

    // Ensure N is an exact multiple of E (and E > 0)
    template<std::size_t E, std::size_t N>
    concept multiple_of = (E > 0) && (N % E == 0);
} // namespace mercury

#define IF_LIKELY(x) [[likely]] if (x)
#define IF_UNLIKELY(x) [[unlikely]] if (x)