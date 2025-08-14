#include "vk_utils.h"

#ifdef MERCURY_LL_GRAPHICS_VULKAN
#include "vk_graphics.h"

static std::tuple<VkPipelineStageFlags2, VkAccessFlags2> makePipelineStageAccessTuple(VkImageLayout state)
{
	switch (state)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		return std::make_tuple(VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE);
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		return std::make_tuple(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		return std::make_tuple(VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
			| VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT,
			VK_ACCESS_2_SHADER_READ_BIT);
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		return std::make_tuple(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
	case VK_IMAGE_LAYOUT_GENERAL:
		return std::make_tuple(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT);
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		return std::make_tuple(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_NONE);
	default: {
		return std::make_tuple(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT);
	}
	}
};

static VkImageMemoryBarrier2 createImageMemoryBarrier(VkImage       image,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT,
												0, 1, 0, 1 })
{
	const auto [srcStage, srcAccess] = makePipelineStageAccessTuple(oldLayout);
	const auto [dstStage, dstAccess] = makePipelineStageAccessTuple(newLayout);

	VkImageMemoryBarrier2 barrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
								  .srcStageMask = srcStage,
								  .srcAccessMask = srcAccess,
								  .dstStageMask = dstStage,
								  .dstAccessMask = dstAccess,
								  .oldLayout = oldLayout,
								  .newLayout = newLayout,
								  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
								  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
								  .image = image,
								  .subresourceRange = subresourceRange };
	return barrier;
}

void vk_utils::ImageTransition(VkCommandBuffer cbuff, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask, int numMips, int numLayers)
{
	const VkImageMemoryBarrier2 barrier = createImageMemoryBarrier(image, oldLayout, newLayout, { aspectMask, 0, (uint32_t)numMips, 0, (uint32_t)numLayers });
	const VkDependencyInfo depInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier };

	vkCmdPipelineBarrier2(cbuff, &depInfo);
}

void vk_utils::BufferMemoryBarrier(VkCommandBuffer cbuff, VkBuffer buffer, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)
{
	VkBufferMemoryBarrier bufferBarrier = {};
	bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferBarrier.srcAccessMask = srcAccess;
	bufferBarrier.dstAccessMask = dstAccess;
	bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferBarrier.buffer = buffer;
	bufferBarrier.size = VK_WHOLE_SIZE;
	vkCmdPipelineBarrier(cbuff, srcStage, dstStage, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
}

VkBufferImageCopy vk_utils::MakeBufferImageCopy(VkExtent3D extent, VkImageAspectFlags aspectMask, int numMips, int numLayers)
{
	VkBufferImageCopy copyRegion = {};
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;
	copyRegion.imageSubresource.aspectMask = aspectMask;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;
	copyRegion.imageOffset = { 0, 0, 0 };
	copyRegion.imageExtent = extent;
	return copyRegion;
}

mercury::ll::graphics::AdapterInfo::Vendor GetVendorFromVkVendorID(mercury::u64 vendor_id)
{
    
    switch (vendor_id)
    {
    case 0x10DE: return mercury::ll::graphics::AdapterInfo::Vendor::NVIDIA;
    case 0x1002: return mercury::ll::graphics::AdapterInfo::Vendor::AMD;
    case 0x8086: return mercury::ll::graphics::AdapterInfo::Vendor::Intel;
    case 0x13B5: return mercury::ll::graphics::AdapterInfo::Vendor::Qualcomm;
    case 0x1AE0: return mercury::ll::graphics::AdapterInfo::Vendor::ARM;
    default: return mercury::ll::graphics::AdapterInfo::Vendor::Unknown;
    }
}

void vk_utils::debug::_setObjectName(mercury::u64 objHandle, VkObjectType objType, const char* name)
{
	if (vkSetDebugUtilsObjectNameEXT)
	{
		VkDebugUtilsObjectNameInfoEXT info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		info.objectHandle = objHandle;
		info.objectType = objType;
		info.pObjectName = name;

		VK_CALL(vkSetDebugUtilsObjectNameEXT(gVKDevice, &info));
	}
}

#endif