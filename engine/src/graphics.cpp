#include "graphics.h"
#include "ll/os.h"
#include <algorithm>
#include "mercury_log.h"
#include "ll/graphics/mercury_swapchain.h"
#include "imgui/mercury_imgui.h"

using namespace mercury;
using namespace ll::graphics;

// Global graphics objects
Instance *mercury::ll::graphics::gInstance = nullptr;
Device *mercury::ll::graphics::gDevice = nullptr;
Adapter *mercury::ll::graphics::gAdapter = nullptr;
Swapchain *mercury::ll::graphics::gSwapchain = nullptr;

void MercuryGraphicsInitialize()
{
    auto &appCfg = Application::GetCurrentApplication()->GetConfig();
    auto &graphicsCfg = appCfg.graphics;

    mercurySwapchainConfigure(appCfg.swapchain);
    gInstance = new Instance();
    gInstance->Initialize();

    AdapterSelectorInfo selector_info = {};
    selector_info.adapter_type_preference = graphicsCfg.adapterPreference;
    selector_info.adapter_index = graphicsCfg.explicitAdapterIndex;
    gInstance->AcquireAdapter(selector_info);

    gAdapter->Initialize();

    gAdapter->CreateDevice();
    gDevice->Initialize();

    //TODO: think about waiting the surface to ready
    if (gSwapchain == nullptr )
    {
        while (!ll::os::gOS->GetCurrentNativeWindowHandle())
        {
            ll::os::gOS->Update();
            ll::os::gOS->Sleep(1);
        }

        gDevice->InitializeSwapchain();
    }                

    mercury_imgui::Initialize();
}

void MercuryGraphicsShutdown()
{
    mercury_imgui::Shutdown();
    
    gDevice->Shutdown();
    delete gDevice;
    gDevice = nullptr;

    gAdapter->Shutdown();
    delete gAdapter;
    gAdapter = nullptr;

    gInstance->Shutdown();
    delete gInstance;
    gInstance = nullptr;
}

void MercuryGraphicsTick()
{
    IF_LIKELY(gDevice)
    {
        IF_UNLIKELY(ll::os::gOS->GetCurrentNativeWindowHandle() == nullptr)
        {
            if (gSwapchain != nullptr)
            {
                gDevice->ShutdownSwapchain();
            }
        }
        else
        {
            if (gSwapchain == nullptr)
                gDevice->InitializeSwapchain();
        }

        IF_LIKELY(gSwapchain)
        {
            auto finalCmdList = gSwapchain->AcquireNextImage();


            mercury_imgui::BeginFrame(finalCmdList);

            mercury_imgui::EndFrame(finalCmdList);
            // do all graphics job here
            gSwapchain->Present();
        }

        gDevice->Tick();
    }
}

std::vector<mercury::ll::graphics::AdapterInfo> gAllAdaptersInfo;

u8 Instance::GetAdapterCount()
{
    return static_cast<u8>(gAllAdaptersInfo.size());
}

/// @brief Get information about the adapter at the specified index. Use it for custom adapter selection.
const AdapterInfo &Instance::GetAdapterInfo(u8 index) const
{
    MERCURY_ASSERT(index < gAllAdaptersInfo.size());

    return gAllAdaptersInfo[index];
}

u64 CalculateAdapterScore(const AdapterInfo &adapter)
{
    u64 totalScore = 0;

    if(adapter.type == AdapterInfo::Type::Discrete)
    {
        totalScore += 1000;
    }

    if(adapter.type == AdapterInfo::Type::Integrated)
    {
        totalScore += 200;
    }

    return totalScore;
}

mercury::u8 SelectAdapterByHeuristic(
    const mercury::ll::graphics::AdapterSelectorInfo &selector_info)
{
    if(selector_info.adapter_index != 255)
        return selector_info.adapter_index;

    struct AdapterRank
    {
        u8 index = 0;
        i64 score = 0;

        AdapterInfo* info = nullptr;
    };

    std::vector<AdapterRank> rankedAdapters;

    for(u8 i = 0; i < gAllAdaptersInfo.size(); ++i)
    {     
        if(true /*TODO: Filter by specific adapter features*/)
        {
            AdapterRank rank;
            rank.index = i;
            rank.info = &gAllAdaptersInfo[i];
            rank.score = CalculateAdapterScore(gAllAdaptersInfo[i]);
            rankedAdapters.push_back(rank);
        }
    }

    if(rankedAdapters.empty())
    {
        MLOG_ERROR(u8"No suitable adapters found for selection.");
        return 0; // No adapters available
    }

    if(selector_info.adapter_type_preference == Config::Graphics::AdapterTypePreference::Any)
    {
        // sort by adapter index (Let OS decide)
        std::sort(rankedAdapters.begin(), rankedAdapters.end(), [](const AdapterRank &a, const AdapterRank &b) {
            return a.index < b.index;
        });

        return rankedAdapters.front().index;
    }
   
    if(selector_info.adapter_type_preference == Config::Graphics::AdapterTypePreference::HighPerformance)
    {
        // sort by score (prefer discrete adapters)
        std::sort(rankedAdapters.begin(), rankedAdapters.end(), [](const AdapterRank &a, const AdapterRank &b) {
            return a.score > b.score;
        });

        return rankedAdapters.front().index;
    }

    if(selector_info.adapter_type_preference == Config::Graphics::AdapterTypePreference::LowPower)
    {
        // sort by score (prefer integrated adapters)
        std::sort(rankedAdapters.begin(), rankedAdapters.end(), [](const AdapterRank &a, const AdapterRank &b) {
            return a.score < b.score;
        });

        return rankedAdapters.front().index;
    }

    return 0;
}