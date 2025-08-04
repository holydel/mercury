#pragma once

#include "mercury_api.h"

namespace mercury {

// SIMD utility functions for cross-platform compatibility
namespace simd {

// Float32x4 operations
inline f32x4 make_f32x4(f32 x, f32 y, f32 z, f32 w) {
#if defined(__ARM_NEON)
    return vsetq_lane_f32(w, vsetq_lane_f32(z, vsetq_lane_f32(y, vsetq_lane_f32(x, vdupq_n_f32(0.0f), 0), 1), 2), 3);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_f32x4_make(x, y, z, w);
#else
    return _mm_setr_ps(x, y, z, w);
#endif
}

inline f32x4 add_f32x4(f32x4 a, f32x4 b) {
#if defined(__ARM_NEON)
    return vaddq_f32(a, b);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_f32x4_add(a, b);
#else
    return _mm_add_ps(a, b);
#endif
}

inline f32x4 sub_f32x4(f32x4 a, f32x4 b) {
#if defined(__ARM_NEON)
    return vsubq_f32(a, b);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_f32x4_sub(a, b);
#else
    return _mm_sub_ps(a, b);
#endif
}

inline f32x4 mul_f32x4(f32x4 a, f32x4 b) {
#if defined(__ARM_NEON)
    return vmulq_f32(a, b);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_f32x4_mul(a, b);
#else
    return _mm_mul_ps(a, b);
#endif
}

inline f32x4 div_f32x4(f32x4 a, f32x4 b) {
#if defined(__ARM_NEON)
    return vdivq_f32(a, b);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_f32x4_div(a, b);
#else
    return _mm_div_ps(a, b);
#endif
}

template<int LaneIndex>
inline f32 extract_lane_f32x4(f32x4 v) {
#if defined(__ARM_NEON)
    return vgetq_lane_f32(v, LaneIndex);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_f32x4_extract_lane(v, LaneIndex);
#else
    return _mm_cvtss_f32(_mm_shuffle_ps(v, v, _MM_SHUFFLE(LaneIndex, LaneIndex, LaneIndex, LaneIndex)));
#endif
}

// Int32x4 operations
inline i32x4 make_i32x4(i32 x, i32 y, i32 z, i32 w) {
#if defined(__ARM_NEON)
    return vsetq_lane_s32(w, vsetq_lane_s32(z, vsetq_lane_s32(y, vsetq_lane_s32(x, vdupq_n_s32(0), 0), 1), 2), 3);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_i32x4_make(x, y, z, w);
#else
    return _mm_setr_epi32(x, y, z, w);
#endif
}

inline i32x4 add_i32x4(i32x4 a, i32x4 b) {
#if defined(__ARM_NEON)
    return vaddq_s32(a, b);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_i32x4_add(a, b);
#else
    return _mm_add_epi32(a, b);
#endif
}

inline i32x4 sub_i32x4(i32x4 a, i32x4 b) {
#if defined(__ARM_NEON)
    return vsubq_s32(a, b);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_i32x4_sub(a, b);
#else
    return _mm_sub_epi32(a, b);
#endif
}

inline i32x4 mul_i32x4(i32x4 a, i32x4 b) {
#if defined(__ARM_NEON)
    return vmulq_s32(a, b);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_i32x4_mul(a, b);
#else
    return _mm_mullo_epi32(a, b);
#endif
}

template<int LaneIndex>
inline i32 extract_lane_i32x4(i32x4 v) {
#if defined(__ARM_NEON)
    return vgetq_lane_s32(v, LaneIndex);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_i32x4_extract_lane(v, LaneIndex);
#else
    return _mm_extract_epi32(v, LaneIndex);
#endif
}

// Float64x2 operations
inline f64x2 make_f64x2(f64 x, f64 y) {
#if defined(__ARM_NEON)
    return vsetq_lane_f64(y, vsetq_lane_f64(x, vdupq_n_f64(0.0), 0), 1);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_f64x2_make(x, y);
#else
    return _mm_setr_pd(x, y);
#endif
}

inline f64x2 add_f64x2(f64x2 a, f64x2 b) {
#if defined(__ARM_NEON)
    return vaddq_f64(a, b);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_f64x2_add(a, b);
#else
    return _mm_add_pd(a, b);
#endif
}

inline f64x2 sub_f64x2(f64x2 a, f64x2 b) {
#if defined(__ARM_NEON)
    return vsubq_f64(a, b);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_f64x2_sub(a, b);
#else
    return _mm_sub_pd(a, b);
#endif
}

inline f64x2 mul_f64x2(f64x2 a, f64x2 b) {
#if defined(__ARM_NEON)
    return vmulq_f64(a, b);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_f64x2_mul(a, b);
#else
    return _mm_mul_pd(a, b);
#endif
}

inline f64x2 div_f64x2(f64x2 a, f64x2 b) {
#if defined(__ARM_NEON)
    return vdivq_f64(a, b);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_f64x2_div(a, b);
#else
    return _mm_div_pd(a, b);
#endif
}

template<int LaneIndex>
inline f64 extract_lane_f64x2(f64x2 v) {
#if defined(__ARM_NEON)
    return vgetq_lane_f64(v, LaneIndex);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_f64x2_extract_lane(v, LaneIndex);
#else
    return _mm_cvtsd_f64(_mm_shuffle_pd(v, v, _MM_SHUFFLE2(LaneIndex, LaneIndex)));
#endif
}

// Utility functions
inline f32x4 load_f32x4(const f32* ptr) {
#if defined(__ARM_NEON)
    return vld1q_f32(ptr);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_v128_load(ptr);
#else
    return _mm_loadu_ps(ptr);
#endif
}

inline void store_f32x4(f32* ptr, f32x4 v) {
#if defined(__ARM_NEON)
    vst1q_f32(ptr, v);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    wasm_v128_store(ptr, v);
#else
    _mm_storeu_ps(ptr, v);
#endif
}

inline f32x4 splat_f32x4(f32 value) {
#if defined(__ARM_NEON)
    return vdupq_n_f32(value);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_f32x4_splat(value);
#else
    return _mm_set1_ps(value);
#endif
}

inline i32x4 splat_i32x4(i32 value) {
#if defined(__ARM_NEON)
    return vdupq_n_s32(value);
#elif defined(MERCURY_LL_OS_EMSCRIPTEN)
    return wasm_i32x4_splat(value);
#else
    return _mm_set1_epi32(value);
#endif
}

} // namespace simd

} // namespace mercury 