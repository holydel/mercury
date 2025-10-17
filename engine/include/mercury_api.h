#pragma once

#include <type_traits>
#include <string>

// Feature availability macros
#if MERCURY_UWP
#define MERCURY_USE_CPP20_FEATURES 0
#else
#define MERCURY_USE_CPP20_FEATURES 1
#endif

// Conditional includes
#if MERCURY_USE_CPP20_FEATURES
#include <concepts>
#endif

// Platform detection
#ifndef MERCURY_LL_OS_EMSCRIPTEN
#if defined(__EMSCRIPTEN__)
#define MERCURY_LL_OS_EMSCRIPTEN
#endif
#endif

#ifndef MERCURY_LL_OS_WIN32
#if defined(_WIN32)
#define MERCURY_LL_OS_WIN32
#endif
#endif

#ifndef MERCURY_LL_OS_MACOS
#if defined(__APPLE__)
#define MERCURY_LL_OS_MACOS
#endif
#endif

#ifndef MERCURY_LL_OS_LINUX
#if defined(__linux__) && !defined(__ANDROID__)
#define MERCURY_LL_OS_LINUX
#endif
#endif

#ifndef MERCURY_LL_OS_ANDROID
#if defined(__ANDROID__)
#define MERCURY_LL_OS_ANDROID
#endif
#endif

// SIMD support includes
#if defined(__aarch64__)
#include <arm_neon.h>
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
// Emscripten WebAssembly SIMD support
#include <wasm_simd128.h>
#elif defined(MERCURY_LL_OS_WIN32) || defined(MERCURY_LL_OS_LINUX) || \
    defined(MERCURY_LL_OS_MACOS)
#include <immintrin.h>
#endif

// Graphics API selection
//#define MERCURY_LL_GRAPHICS_NULL
//#define MERCURY_LL_GRAPHICS_VULKAN
//#define MERCURY_LL_GRAPHICS_D3D12
//#define MERCURY_LL_GRAPHICS_METAL
#define MERCURY_LL_GRAPHICS_WEBGPU

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
#if defined(__aarch64__)
    // ARM64 NEON SIMD types
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
    #if MERCURY_USE_CPP20_FEATURES
    typedef char8_t c8;    
    #else
    typedef char c8;
    #endif

	typedef std::basic_string<c8> c8string;
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

    struct PackedColor
    {
        union
        {
            struct
            {
                u8 r, g, b, a;
            };
			u32 rgba = 0xffffffff;
        };
        
        PackedColor() = default;
        PackedColor(u8 red, u8 green, u8 blue, u8 alpha = 255)
			: r(red), g(green), b(blue), a(alpha) {
		}
    };
#if MERCURY_USE_CPP20_FEATURES
    // C++20 version with concepts
    template<std::unsigned_integral T>
    struct Handle
    {
        static constexpr T InvalidValue = (T)-1;
        T handle = InvalidValue;

        bool isValid() const { return handle != InvalidValue; }
        T Value() const { return handle; }
        void Invalidate() { handle = InvalidValue; }

        template<typename U>
        Handle<T>& operator=(U value) requires std::is_convertible_v<U, T>
        {
            handle = static_cast<T>(value);
            return *this;
        }
    };
#else
    // C++17 version with SFINAE
    template<typename T, typename = std::enable_if_t<
        std::is_unsigned_v<T>&& std::is_integral_v<T>>>
        struct Handle
    {
        static constexpr T InvalidValue = (T)-1;
        T handle = InvalidValue;

        bool isValid() const { return handle != InvalidValue; }
        T Value() const { return handle; }
        void Invalidate() { handle = InvalidValue; }

        template<typename U, typename = std::enable_if_t<
            std::is_convertible_v<U, T>>>
            Handle<T>& operator=(U value)
        {
            handle = static_cast<T>(value);
            return *this;
        }
    };
#endif

    // Ensure N is an exact multiple of E (and E > 0)
    #if MERCURY_USE_CPP20_FEATURES
        template<std::size_t E, std::size_t N>
        concept multiple_of = (E > 0) && (N % E == 0);
    #else
        // C++17: Use static_assert at usage site instead
    #define MERCURY_ASSERT_MULTIPLE_OF(E, N) \
            static_assert((E) > 0 && (N) % (E) == 0, \
                          "N must be a multiple of E")
#endif
} // namespace mercury

#if MERCURY_USE_CPP20_FEATURES
#define MSTR(str) u8##str
#else
#define MSTR(str) reinterpret_cast<const mercury::c8*>(u8##str)
#endif

#if MERCURY_USE_CPP20_FEATURES
#define IF_LIKELY(x) [[likely]] if (x)
#define IF_UNLIKELY(x) [[unlikely]] if (x)
#elif defined(_MSC_VER)
#define IF_LIKELY(x) if (x)
#define IF_UNLIKELY(x) if (x)
#elif defined(__GNUC__) || defined(__clang__)
#define IF_LIKELY(x) if (__builtin_expect(!!(x), 1))
#define IF_UNLIKELY(x) if (__builtin_expect(!!(x), 0))
#else
#define IF_LIKELY(x) if (x)
#define IF_UNLIKELY(x) if (x)
#endif
