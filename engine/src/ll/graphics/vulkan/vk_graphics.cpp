#include "ll/graphics.h"

#if defined(MERCURY_LL_GRAPHICS_VULKAN)

using namespace mercury;
using namespace mercury::ll::graphics;

#include "vk_graphics.h"
#include "vk_utils.h"

mercury::Config::VKConfig gVKConfig;

VkRenderPass gVKFinalRenderPass = VK_NULL_HANDLE;

const char* ll::graphics::GetBackendName()
{
    static const char* backendName = "Vulkan";
    return backendName;
}

void TimelineSemaphore::WaitUntil(mercury::u64 value, mercury::u64 timeout)
{
    VkSemaphoreWaitInfo waitInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores = reinterpret_cast<VkSemaphore*>(&nativePtr),
        .pValues = &value,
    };

    vkWaitSemaphores(gVKDevice, &waitInfo, timeout);
}

void TimelineSemaphore::SetDebugName(const char *utf8_name)
{
    vk_utils::debug::SetName(static_cast<VkSemaphore>(nativePtr), utf8_name);
}

void TimelineSemaphore::Destroy()
{
    vkDestroySemaphore(gVKDevice, static_cast<VkSemaphore>(nativePtr), gVKGlobalAllocationsCallbacks);
    nativePtr = nullptr;
}

void RenderPass::SetDebugName(const char *utf8_name)
{
    vk_utils::debug::SetName(static_cast<VkRenderPass>(nativePtr), utf8_name);
}

void RenderPass::Destroy()
{
    vkDestroyRenderPass(gVKDevice, static_cast<VkRenderPass>(nativePtr), gVKGlobalAllocationsCallbacks);
    nativePtr = nullptr;
}

bool CommandList::IsExecuted()
{
    return false;
}

void CommandList::SetDebugName(const char *utf8_name)
{
    vk_utils::debug::SetName(static_cast<VkCommandBuffer>(nativePtr), utf8_name);
}

void CommandList::Destroy()
{
    vkFreeCommandBuffers(gVKDevice, static_cast<VkCommandPool>(nativePtr), 1, reinterpret_cast<VkCommandBuffer*>(&nativePtr));
    nativePtr = nullptr;
}

CommandList CommandPool::AllocateCommandList()
{
    CommandList result;

    const VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = static_cast<VkCommandPool>(nativePtr),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VK_CALL(vkAllocateCommandBuffers(gVKDevice, &commandBufferAllocateInfo, reinterpret_cast<VkCommandBuffer*>(&result.nativePtr)));

    return result;
}

void CommandPool::SetDebugName(const char *utf8_name)
{
    vk_utils::debug::SetName(static_cast<VkCommandPool>(nativePtr), utf8_name);
}

void CommandPool::Destroy()
{
    vkDestroyCommandPool(gVKDevice, static_cast<VkCommandPool>(nativePtr), gVKGlobalAllocationsCallbacks);
    nativePtr = nullptr;
}

void CommandPool::Reset()
{
    VK_CALL(vkResetCommandPool(gVKDevice, static_cast<VkCommandPool>(nativePtr), 0));
}


#endif