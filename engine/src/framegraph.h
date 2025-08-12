#pragma once
#include "mercury_framegraph.h"

namespace mercury
{
    namespace framegraph
    {
        void Initialize(mercury::u8 numberFramesInFlight = 3);
        void Shutdown();

        void BeginFrame();
        void EndFrame();
        void SubmitFrame();
        void PresentFrame();
    }
}