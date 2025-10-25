#include "mercury.h"
#include "mercury_entry_point.h"
#include <stdio.h>

#include "ll/os.h"
#include "ll/graphics.h"

#include "mercury_embedded_shaders.h"
#include "mercury_shader.h"

#include <mercury_canvas.h>

using namespace mercury;

//void test_simd() {
//       printf("Mercury Engine SIMD Test\n");
//
//       printf("SIMD Platform: ");
//   
//       #if defined(MERCURY_LL_OS_EMSCRIPTEN)
//       printf("Emscripten (WebAssembly SIMD)");
//   #elif defined(__ARM_NEON)
//       printf("ARM NEON");
//   #elif defined(MERCURY_LL_OS_WIN32) || defined(MERCURY_LL_OS_LINUX) || defined(MERCURY_LL_OS_MACOS)
//       printf("x86/x64 SSE");
//   #else
//       printf("Unknown");
//   #endif
//       printf("\n\n");
//       
//       // Test f32x4 operations
//       printf("Testing f32x4 operations:\n");
//       
//       auto v1 = mercury::simd::make_f32x4(1.0f, 2.0f, 3.0f, 4.0f);
//       auto v2 = mercury::simd::make_f32x4(5.0f, 6.0f, 7.0f, 8.0f);
//       
//       auto v_add = mercury::simd::add_f32x4(v1, v2);
//       auto v_sub = mercury::simd::sub_f32x4(v2, v1);
//       auto v_mul = mercury::simd::mul_f32x4(v1, v2);
//       auto v_div = mercury::simd::div_f32x4(v2, v1);
//       
//       printf("v1: [%.1f, %.1f, %.1f, %.1f]\n", 
//              mercury::simd::extract_lane_f32x4<0>(v1),
//              mercury::simd::extract_lane_f32x4<1>(v1),
//              mercury::simd::extract_lane_f32x4<2>(v1),
//              mercury::simd::extract_lane_f32x4<3>(v1));
//       
//       printf("v2: [%.1f, %.1f, %.1f, %.1f]\n", 
//              mercury::simd::extract_lane_f32x4<0>(v2),
//              mercury::simd::extract_lane_f32x4<1>(v2),
//              mercury::simd::extract_lane_f32x4<2>(v2),
//              mercury::simd::extract_lane_f32x4<3>(v2));
//       
//       printf("v1 + v2: [%.1f, %.1f, %.1f, %.1f]\n", 
//              mercury::simd::extract_lane_f32x4<0>(v_add),
//              mercury::simd::extract_lane_f32x4<1>(v_add),
//              mercury::simd::extract_lane_f32x4<2>(v_add),
//              mercury::simd::extract_lane_f32x4<3>(v_add));
//       
//       printf("v2 - v1: [%.1f, %.1f, %.1f, %.1f]\n", 
//              mercury::simd::extract_lane_f32x4<0>(v_sub),
//              mercury::simd::extract_lane_f32x4<1>(v_sub),
//              mercury::simd::extract_lane_f32x4<2>(v_sub),
//              mercury::simd::extract_lane_f32x4<3>(v_sub));
//       
//       printf("v1 * v2: [%.1f, %.1f, %.1f, %.1f]\n", 
//              mercury::simd::extract_lane_f32x4<0>(v_mul),
//              mercury::simd::extract_lane_f32x4<1>(v_mul),
//              mercury::simd::extract_lane_f32x4<2>(v_mul),
//              mercury::simd::extract_lane_f32x4<3>(v_mul));
//       
//       printf("v2 / v1: [%.1f, %.1f, %.1f, %.1f]\n", 
//              mercury::simd::extract_lane_f32x4<0>(v_div),
//              mercury::simd::extract_lane_f32x4<1>(v_div),
//              mercury::simd::extract_lane_f32x4<2>(v_div),
//              mercury::simd::extract_lane_f32x4<3>(v_div));
//   
//       // Test i32x4 operations
//       printf("\nTesting i32x4 operations:\n");
//       
//       auto iv1 = mercury::simd::make_i32x4(10, 20, 30, 40);
//       auto iv2 = mercury::simd::make_i32x4(5, 6, 7, 8);
//       
//       auto iv_add = mercury::simd::add_i32x4(iv1, iv2);
//       auto iv_sub = mercury::simd::sub_i32x4(iv1, iv2);
//       auto iv_mul = mercury::simd::mul_i32x4(iv1, iv2);
//       
//       printf("iv1: [%d, %d, %d, %d]\n", 
//              mercury::simd::extract_lane_i32x4<0>(iv1),
//              mercury::simd::extract_lane_i32x4<1>(iv1),
//              mercury::simd::extract_lane_i32x4<2>(iv1),
//              mercury::simd::extract_lane_i32x4<3>(iv1));
//       
//       printf("iv2: [%d, %d, %d, %d]\n", 
//              mercury::simd::extract_lane_i32x4<0>(iv2),
//              mercury::simd::extract_lane_i32x4<1>(iv2),
//              mercury::simd::extract_lane_i32x4<2>(iv2),
//              mercury::simd::extract_lane_i32x4<3>(iv2));
//       
//       printf("iv1 + iv2: [%d, %d, %d, %d]\n", 
//              mercury::simd::extract_lane_i32x4<0>(iv_add),
//              mercury::simd::extract_lane_i32x4<1>(iv_add),
//              mercury::simd::extract_lane_i32x4<2>(iv_add),
//              mercury::simd::extract_lane_i32x4<3>(iv_add));
//       
//       printf("iv1 - iv2: [%d, %d, %d, %d]\n", 
//              mercury::simd::extract_lane_i32x4<0>(iv_sub),
//              mercury::simd::extract_lane_i32x4<1>(iv_sub),
//              mercury::simd::extract_lane_i32x4<2>(iv_sub),
//              mercury::simd::extract_lane_i32x4<3>(iv_sub));
//       
//       printf("iv1 * iv2: [%d, %d, %d, %d]\n", 
//              mercury::simd::extract_lane_i32x4<0>(iv_mul),
//              mercury::simd::extract_lane_i32x4<1>(iv_mul),
//              mercury::simd::extract_lane_i32x4<2>(iv_mul),
//              mercury::simd::extract_lane_i32x4<3>(iv_mul));
//   
//       // Test f64x2 operations
//       printf("\nTesting f64x2 operations:\n");
//       
//       auto dv1 = mercury::simd::make_f64x2(1.5, 2.5);
//       auto dv2 = mercury::simd::make_f64x2(3.5, 4.5);
//       
//       auto dv_add = mercury::simd::add_f64x2(dv1, dv2);
//       auto dv_mul = mercury::simd::mul_f64x2(dv1, dv2);
//       
//       printf("dv1: [%.1f, %.1f]\n", 
//              mercury::simd::extract_lane_f64x2<0>(dv1),
//              mercury::simd::extract_lane_f64x2<1>(dv1));
//       
//       printf("dv2: [%.1f, %.1f]\n", 
//              mercury::simd::extract_lane_f64x2<0>(dv2),
//              mercury::simd::extract_lane_f64x2<1>(dv2));
//       
//       printf("dv1 + dv2: [%.1f, %.1f]\n", 
//              mercury::simd::extract_lane_f64x2<0>(dv_add),
//              mercury::simd::extract_lane_f64x2<1>(dv_add));
//       
//       printf("dv1 * dv2: [%.1f, %.1f]\n", 
//              mercury::simd::extract_lane_f64x2<0>(dv_mul),
//              mercury::simd::extract_lane_f64x2<1>(dv_mul));
//   
//       // Test memory operations
//       printf("\nTesting memory operations:\n");
//       
//       mercury::f32 data[4] = {1.1f, 2.2f, 3.3f, 4.4f};
//       auto loaded = mercury::simd::load_f32x4(data);
//       
//       printf("Loaded from memory: [%.1f, %.1f, %.1f, %.1f]\n", 
//              mercury::simd::extract_lane_f32x4<0>(loaded),
//              mercury::simd::extract_lane_f32x4<1>(loaded),
//              mercury::simd::extract_lane_f32x4<2>(loaded),
//              mercury::simd::extract_lane_f32x4<3>(loaded));
//       
//       mercury::f32 result[4];
//       mercury::simd::store_f32x4(result, loaded);
//       printf("Stored to memory: [%.1f, %.1f, %.1f, %.1f]\n", 
//              result[0], result[1], result[2], result[3]);
//   
//
//       printf("\nSIMD test completed successfully!\n"); 
//}
//
//void test_memory()
//{
//       mercury::memory::ReservedAllocator::InitDesc initDesc;
//
//       initDesc.bucketsInfo.push_back({16, 16_MB, 1_MB});
//       initDesc.bucketsInfo.push_back({32, 16_MB, 1_MB});
//       initDesc.bucketsInfo.push_back({64, 16_MB, 1_MB});
//       initDesc.bucketsInfo.push_back({128, 16_MB, 1_MB});
//       initDesc.bucketsInfo.push_back({256, 16_MB, 1_MB});
//       initDesc.bucketsInfo.push_back({512, 16_MB, 1_MB});
//       initDesc.bucketsInfo.push_back({1024, 16_MB, 1_MB});
//       initDesc.bucketsInfo.push_back({2048, 16_MB, 1_MB});
//       initDesc.bucketsInfo.push_back({4096, 16_MB, 1_MB});
//
//       mercury::memory::ReservedAllocator allocator(initDesc);
//
//       {
//              void* ptr = allocator.Allocate(128 * 1024);
//              allocator.Deallocate(ptr);
//       }   
//       
//       {
//              void* ptr = allocator.Allocate(7);
//              allocator.Deallocate(ptr);
//       }  
//   
//       {
//              void* ptr = allocator.Allocate(5);
//              allocator.Deallocate(ptr);
//       }  
//
//       {
//              void* ptr = allocator.Allocate(2000);
//              allocator.Deallocate(ptr);
//       }  
//
//       {
//              void* ptr = allocator.Allocate(100);
//              allocator.Deallocate(ptr);
//       }  
//
//       //allocator.DumpStatsPerBucketTotal();
//}


class TestBedApplication : public Application {

       bool m_running = true;
       int frameCount = 0;
       float t = 0;
	   mercury::ll::graphics::PsoHandle testTrianglePSO;
       mercury::ll::graphics::ShaderHandle testTriangleVS;
       mercury::ll::graphics::ShaderHandle testTriangleFS;
public:
       void Configure() override
       {
              MLOG_DEBUG(u8"TestBedApplication::Configure");
              config.appName = u8"TestBed Application";
              config.appVersion = Config::Version{{{0, 1, 0}}};
              config.window.width = 1600;
              config.window.height = 900;
              config.window.resizable = true;
              config.graphics.enableValidationLayers = true; // Enable validation layers for testing
              config.graphics.adapterPreference = Config::Graphics::AdapterTypePreference::HighPerformance;

              config.logger.logToConsole = true;
              config.logger.logToIDE = false;
       }
       void Initialize() override;
       void Tick() override;
       void Shutdown() override;
       void OnFinalPass(mercury::ll::graphics::CommandList& finalCL) override;
       void OnClose() override { m_running = false; }
          bool IsRunning() override { 
                 // Return running flag without verbose logging
                 return m_running; 
          }
};

TestBedApplication gApplication;

void TestBedApplication::Initialize() {
       MLOG_DEBUG(u8"TestBedApplication::Initialize SOME CHANGES 2");
       //test_simd();
       //test_memory();

       memory::gGraphicsMemoryAllocator->DumpStatsPerBucketTotal();
       testTriangleVS = ll::graphics::gDevice->CreateShaderModule(ll::graphics::embedded_shaders::TestTriangleRotatedVS());
       testTriangleFS = ll::graphics::gDevice->CreateShaderModule(ll::graphics::embedded_shaders::TestTrianglePS());

	   //ll::graphics::RasterizePipelineDescriptor psoDesc = {};
	   //psoDesc.vertexShader = testTriangleVS;
	   //psoDesc.fragmentShader = testTriangleFS;

    //   psoDesc.pushConstantSize = 4; //float angle

	   //testTrianglePSO = ll::graphics::gDevice->CreateRasterizePipeline(psoDesc);
}

void TestBedApplication::Tick() {
      t += 0.1f;

      if(ll::graphics::gSwapchain)
      {
       ll::graphics::gSwapchain->clearColor = glm::vec4(sin(t)*0.5f + 0.5f,cos(t)*0.5f + 0.5f,0.33f,1.0f);
       // ll::graphics::gSwapchain->clearColor = glm::vec4(0.02f,0.03f,0.05f,1.0f);
      }      

      frameCount++;

      if(input::gKeyboard->IsKeyPressed(input::Key::Escape)) {
              MLOG_DEBUG(u8"Escape key pressed, shutting down application.");
              m_running = false;
      }

      if(input::gKeyboard->IsKeyPressed(input::Key::Enter)) {
              MLOG_DEBUG(u8"Enter key pressed.");
      }

       if(input::gMouse->IsButtonPressed(input::MouseButton::Left)) {
                MLOG_DEBUG(u8"Left mouse button pressed");
       }
       if(input::gMouse->IsButtonPressed(input::MouseButton::Right)) {
                MLOG_DEBUG(u8"Right mouse button pressed");
       }
       if(input::gMouse->IsButtonPressed(input::MouseButton::Middle)) {
                MLOG_DEBUG(u8"Middle mouse button pressed");
       }
       if(input::gMouse->IsButtonPressed(input::MouseButton::X1)) {
                MLOG_DEBUG(u8"Mouse X1 button pressed");
       }
       if(input::gMouse->IsButtonPressed(input::MouseButton::X2)) {
                MLOG_DEBUG(u8"Mouse X2 button pressed");
       }

       
       float x = 300 + sin(t * 0.1f) * 100;
       float y = 400 + cos(t * 0.1f) * 150;
       canvas::DrawSprite(input::gMouse->GetPosition(), glm::vec2(10, 50), glm::vec2(0, 0), glm::vec2(1, 1), t * 0.1f, ColorWhite);
    //m_running = false;
}

void TestBedApplication::OnFinalPass(mercury::ll::graphics::CommandList& finalCL)
{
    struct MyPc
            {
        float angle;
	} pc;

    pc.angle = t * 0.1f;

   // finalCL.SetPSO(testTrianglePSO);
	//finalCL.PushConstants(pc);    
	//finalCL.Draw(3, 1, 0, 0);

	//finalCL.SetPSO(testDedicatedSpritePSO);
	//DedicatedSpriteParameters spriteParams;
	//spriteParams.position = glm::vec2(100.0f, 100.0f);
	//spriteParams.size = glm::vec2(200.0f, 200.0f);
	//spriteParams.uvs = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
	//spriteParams.angle = t * 0.2f;
	//spriteParams.colorPacked = 0xFFFFFFFF; // White color
	//finalCL.PushConstants(spriteParams);
	//finalCL.Draw(4, 1, 0, 0);
}
void TestBedApplication::Shutdown() {
       MLOG_DEBUG(u8"TestBedApplication::Shutdown");
}