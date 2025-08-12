#include "framegraph.h"
#include "ll/graphics/ll_graphics.h"
#include <cassert>
#include <sstream>

using namespace mercury;
using namespace framegraph;
using namespace ll::graphics;

TimelineSemaphore gFrameGraphSemaphore;
u32 gFrameRingCurrent{0};

struct FrameData
{
    CommandPool cmdPool;
    CommandList cmdList;
    u64 frameIndex = 0;
};

static std::vector<FrameData> gFrames;

void framegraph::Initialize(mercury::u8 numberFramesInFlight)
{
    assert(numberFramesInFlight > 1);
    const uint64_t initialValue = (numberFramesInFlight - 1);

    gFrameGraphSemaphore = gDevice->CreateTimelineSemaphore(initialValue);
    gFrameGraphSemaphore.SetDebugName("FrameGraphSemaphore");

    gFrames.resize(numberFramesInFlight);

    for (mercury::u8 i = 0; i < numberFramesInFlight; ++i)
    {
        auto &f = gFrames[i];
        f.frameIndex = i; // Track frame index for synchronization
        f.cmdPool = gDevice->CreateCommandPool(QueueType::Graphics);
        f.cmdList = f.cmdPool.AllocateCommandList();

        std::stringstream cmdListName;
        cmdListName << "Frame Command List (" << i << ")";
        f.cmdList.SetDebugName(cmdListName.str().c_str());

        std::stringstream cmdPoolName;
        cmdPoolName << "Frame Command Pool (" << i << ")";
        f.cmdPool.SetDebugName(cmdPoolName.str().c_str());  
    }
}

void framegraph::Shutdown()
{
    gFrameGraphSemaphore.Destroy();

    for (auto &f : gFrames)
    {
        f.cmdList.Destroy();
        f.cmdPool.Destroy();
    }
    gFrames.clear();
}

void framegraph::BeginFrame()
{
    gSwapchain->ReInitIfNeeded();

    auto& frame = gFrames[gFrameRingCurrent];

    gFrameGraphSemaphore.WaitUntil(frame.frameIndex,std::numeric_limits<u64>::max());

    gSwapchain->AcquireNextImage();

    frame.cmdPool.Reset();
}

void framegraph::EndFrame()
{
}

void framegraph::SubmitFrame()
{
}

void framegraph::PresentFrame()
{
}