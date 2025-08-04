#include "engine/include/mercury.h"
#include "engine/include/mercury_log.h"
#include <stdio.h>

#include "ll/os.h"

using namespace mercury;

void test_simd() {
       printf("Mercury Engine SIMD Test\n");

       printf("SIMD Platform: ");
   
       #if defined(MERCURY_LL_OS_EMSCRIPTEN)
       printf("Emscripten (WebAssembly SIMD)");
   #elif defined(__ARM_NEON)
       printf("ARM NEON");
   #elif defined(MERCURY_LL_OS_WIN32) || defined(MERCURY_LL_OS_LINUX) || defined(MERCURY_LL_OS_MACOS)
       printf("x86/x64 SSE");
   #else
       printf("Unknown");
   #endif
       printf("\n\n");
       
       // Test f32x4 operations
       printf("Testing f32x4 operations:\n");
       
       auto v1 = mercury::simd::make_f32x4(1.0f, 2.0f, 3.0f, 4.0f);
       auto v2 = mercury::simd::make_f32x4(5.0f, 6.0f, 7.0f, 8.0f);
       
       auto v_add = mercury::simd::add_f32x4(v1, v2);
       auto v_sub = mercury::simd::sub_f32x4(v2, v1);
       auto v_mul = mercury::simd::mul_f32x4(v1, v2);
       auto v_div = mercury::simd::div_f32x4(v2, v1);
       
       printf("v1: [%.1f, %.1f, %.1f, %.1f]\n", 
              mercury::simd::extract_lane_f32x4<0>(v1),
              mercury::simd::extract_lane_f32x4<1>(v1),
              mercury::simd::extract_lane_f32x4<2>(v1),
              mercury::simd::extract_lane_f32x4<3>(v1));
       
       printf("v2: [%.1f, %.1f, %.1f, %.1f]\n", 
              mercury::simd::extract_lane_f32x4<0>(v2),
              mercury::simd::extract_lane_f32x4<1>(v2),
              mercury::simd::extract_lane_f32x4<2>(v2),
              mercury::simd::extract_lane_f32x4<3>(v2));
       
       printf("v1 + v2: [%.1f, %.1f, %.1f, %.1f]\n", 
              mercury::simd::extract_lane_f32x4<0>(v_add),
              mercury::simd::extract_lane_f32x4<1>(v_add),
              mercury::simd::extract_lane_f32x4<2>(v_add),
              mercury::simd::extract_lane_f32x4<3>(v_add));
       
       printf("v2 - v1: [%.1f, %.1f, %.1f, %.1f]\n", 
              mercury::simd::extract_lane_f32x4<0>(v_sub),
              mercury::simd::extract_lane_f32x4<1>(v_sub),
              mercury::simd::extract_lane_f32x4<2>(v_sub),
              mercury::simd::extract_lane_f32x4<3>(v_sub));
       
       printf("v1 * v2: [%.1f, %.1f, %.1f, %.1f]\n", 
              mercury::simd::extract_lane_f32x4<0>(v_mul),
              mercury::simd::extract_lane_f32x4<1>(v_mul),
              mercury::simd::extract_lane_f32x4<2>(v_mul),
              mercury::simd::extract_lane_f32x4<3>(v_mul));
       
       printf("v2 / v1: [%.1f, %.1f, %.1f, %.1f]\n", 
              mercury::simd::extract_lane_f32x4<0>(v_div),
              mercury::simd::extract_lane_f32x4<1>(v_div),
              mercury::simd::extract_lane_f32x4<2>(v_div),
              mercury::simd::extract_lane_f32x4<3>(v_div));
   
       // Test i32x4 operations
       printf("\nTesting i32x4 operations:\n");
       
       auto iv1 = mercury::simd::make_i32x4(10, 20, 30, 40);
       auto iv2 = mercury::simd::make_i32x4(5, 6, 7, 8);
       
       auto iv_add = mercury::simd::add_i32x4(iv1, iv2);
       auto iv_sub = mercury::simd::sub_i32x4(iv1, iv2);
       auto iv_mul = mercury::simd::mul_i32x4(iv1, iv2);
       
       printf("iv1: [%d, %d, %d, %d]\n", 
              mercury::simd::extract_lane_i32x4<0>(iv1),
              mercury::simd::extract_lane_i32x4<1>(iv1),
              mercury::simd::extract_lane_i32x4<2>(iv1),
              mercury::simd::extract_lane_i32x4<3>(iv1));
       
       printf("iv2: [%d, %d, %d, %d]\n", 
              mercury::simd::extract_lane_i32x4<0>(iv2),
              mercury::simd::extract_lane_i32x4<1>(iv2),
              mercury::simd::extract_lane_i32x4<2>(iv2),
              mercury::simd::extract_lane_i32x4<3>(iv2));
       
       printf("iv1 + iv2: [%d, %d, %d, %d]\n", 
              mercury::simd::extract_lane_i32x4<0>(iv_add),
              mercury::simd::extract_lane_i32x4<1>(iv_add),
              mercury::simd::extract_lane_i32x4<2>(iv_add),
              mercury::simd::extract_lane_i32x4<3>(iv_add));
       
       printf("iv1 - iv2: [%d, %d, %d, %d]\n", 
              mercury::simd::extract_lane_i32x4<0>(iv_sub),
              mercury::simd::extract_lane_i32x4<1>(iv_sub),
              mercury::simd::extract_lane_i32x4<2>(iv_sub),
              mercury::simd::extract_lane_i32x4<3>(iv_sub));
       
       printf("iv1 * iv2: [%d, %d, %d, %d]\n", 
              mercury::simd::extract_lane_i32x4<0>(iv_mul),
              mercury::simd::extract_lane_i32x4<1>(iv_mul),
              mercury::simd::extract_lane_i32x4<2>(iv_mul),
              mercury::simd::extract_lane_i32x4<3>(iv_mul));
   
       // Test f64x2 operations
       printf("\nTesting f64x2 operations:\n");
       
       auto dv1 = mercury::simd::make_f64x2(1.5, 2.5);
       auto dv2 = mercury::simd::make_f64x2(3.5, 4.5);
       
       auto dv_add = mercury::simd::add_f64x2(dv1, dv2);
       auto dv_mul = mercury::simd::mul_f64x2(dv1, dv2);
       
       printf("dv1: [%.1f, %.1f]\n", 
              mercury::simd::extract_lane_f64x2<0>(dv1),
              mercury::simd::extract_lane_f64x2<1>(dv1));
       
       printf("dv2: [%.1f, %.1f]\n", 
              mercury::simd::extract_lane_f64x2<0>(dv2),
              mercury::simd::extract_lane_f64x2<1>(dv2));
       
       printf("dv1 + dv2: [%.1f, %.1f]\n", 
              mercury::simd::extract_lane_f64x2<0>(dv_add),
              mercury::simd::extract_lane_f64x2<1>(dv_add));
       
       printf("dv1 * dv2: [%.1f, %.1f]\n", 
              mercury::simd::extract_lane_f64x2<0>(dv_mul),
              mercury::simd::extract_lane_f64x2<1>(dv_mul));
   
       // Test memory operations
       printf("\nTesting memory operations:\n");
       
       mercury::f32 data[4] = {1.1f, 2.2f, 3.3f, 4.4f};
       auto loaded = mercury::simd::load_f32x4(data);
       
       printf("Loaded from memory: [%.1f, %.1f, %.1f, %.1f]\n", 
              mercury::simd::extract_lane_f32x4<0>(loaded),
              mercury::simd::extract_lane_f32x4<1>(loaded),
              mercury::simd::extract_lane_f32x4<2>(loaded),
              mercury::simd::extract_lane_f32x4<3>(loaded));
       
       mercury::f32 result[4];
       mercury::simd::store_f32x4(result, loaded);
       printf("Stored to memory: [%.1f, %.1f, %.1f, %.1f]\n", 
              result[0], result[1], result[2], result[3]);
   

       printf("\nSIMD test completed successfully!\n"); 
}

class TestBedApplication : public Application {
       bool m_running = true;
       int frameCount = 0;
public:
       void Initialize() override;
       void Tick() override;
       void Shutdown() override;

       bool IsRunning() override { return m_running; }
};

TestBedApplication gApplication;

void TestBedApplication::Initialize() {
       MLOG_DEBUG(u8"TestBedApplication::Initialize SOME CHANGES 2");
       //test_simd();
}

void TestBedApplication::Tick() {
      //utils::debug::output_debug_string_to_console(u8"TestBedApplication::Tick");
      frameCount++;
if(ll::os::gOS->IsFocused())
{
      //MLOG_DEBUG(u8"TestBedApplication::Tick: Focused, frameCount: %d", frameCount);
}
else
{
      //MLOG_DEBUG(u8"TestBedApplication::Tick: Not Focused, frameCount: %d", frameCount);
}

}

void TestBedApplication::Shutdown() {
       MLOG_DEBUG(u8"TestBedApplication::Shutdown");
}