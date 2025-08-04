#include "graphics.h"
#include "ll/os.h"

using namespace mercury;
using namespace ll::graphics;

// Global graphics objects
Instance *mercury::ll::graphics::gInstance = nullptr;
Device *mercury::ll::graphics::gDevice = nullptr;
Adapter *mercury::ll::graphics::gAdapter = nullptr;
Swapchain* mercury::ll::graphics::gSwapchain = nullptr;

void InitializeGraphics()
{
    gInstance = new Instance();
    gInstance->Initialize();
  
    gAdapter = gInstance->AcquireAdapter();
    gAdapter->Initialize();
    gDevice = gAdapter->CreateDevice();
    gDevice->Initialize();
}

void ShutdownGraphics()
{
    gDevice->Shutdown();
    delete gDevice;

    gAdapter->Shutdown();
    delete gAdapter;

    gInstance->Shutdown();
    delete gInstance;
}

void TickGraphics()
{
    IF_LIKELY(gDevice)
    {
        IF_UNLIKELY(ll::os::gOS->GetCurrentNativeWindowHandle() == nullptr) {
           if(gSwapchain != nullptr) {
               gDevice->ShutdownSwapchain();
           }
        }
        else {
            if(gSwapchain == nullptr)
                gDevice->InitializeSwapchain(ll::os::gOS->GetCurrentNativeWindowHandle());         
        }

        IF_LIKELY(gSwapchain)
        {
            gSwapchain->AcquireNextImage();

            //do all graphics job here
            gSwapchain->Present();
        }

        gDevice->Tick();
    }  
}