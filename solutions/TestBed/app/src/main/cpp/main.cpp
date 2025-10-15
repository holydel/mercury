#include "mercury.h"
#include "mercury_entry_point.h"
#include <stdio.h>

#include "ll/os.h"
#include "ll/graphics.h"

#include "mercury_embedded_shaders.h"
#include "mercury_shader.h"

using namespace mercury;

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
    }
    void Initialize() override;
    void Tick() override;
    void Shutdown() override;
    void OnFinalPass(mercury::ll::graphics::CommandList& finalCL) override;
    bool IsRunning() override { return m_running; }
};

TestBedApplication gApplication;

void TestBedApplication::Initialize() {
    MLOG_DEBUG(u8"TestBedApplication::Initialize SOME CHANGES 2");
    //test_simd();
    //test_memory();

    //memory::gGraphicsMemoryAllocator->DumpStatsPerBucketTotal();
    testTriangleVS = ll::graphics::gDevice->CreateShaderModule(ll::graphics::embedded_shaders::TestTriangleVS());
    testTriangleFS = ll::graphics::gDevice->CreateShaderModule(ll::graphics::embedded_shaders::TestTrianglePS());

    ll::graphics::RasterizePipelineDescriptor psoDesc = {};
    psoDesc.vertexShader = testTriangleVS;
    psoDesc.fragmentShader = testTriangleFS;

    testTrianglePSO = ll::graphics::gDevice->CreateRasterizePipeline(psoDesc);
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
}

void TestBedApplication::OnFinalPass(mercury::ll::graphics::CommandList& finalCL)
{
    finalCL.SetPSO(testTrianglePSO);
    finalCL.Draw(3, 1, 0, 0);
}
void TestBedApplication::Shutdown() {
    MLOG_DEBUG(u8"TestBedApplication::Shutdown");
}