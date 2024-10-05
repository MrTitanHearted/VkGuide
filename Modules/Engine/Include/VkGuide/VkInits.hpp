#pragma once

#include <VkGuide/Defines.hpp>

namespace vkinit {
    VkCommandPoolCreateInfo GetCommandPoolCreateInfo(std::uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
    VkCommandBufferAllocateInfo GetCommandBufferAllocateInfo(VkCommandPool pool, std::uint32_t count = 1);

    VkFenceCreateInfo GetFenceCreateInfo(VkFenceCreateFlags flags = 0);
    VkSemaphoreCreateInfo GetSemaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

    VkCommandBufferBeginInfo GetCommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);

    VkImageSubresourceRange GetImageSubresourceRange(VkImageAspectFlags aspectMask);

    VkSemaphoreSubmitInfo GetSemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
    VkCommandBufferSubmitInfo GetCommandBufferSubmitInfo(VkCommandBuffer commandBuffer);
    VkSubmitInfo2 GetSubmitInfo(const VkCommandBufferSubmitInfo &commandBufferInfo, VkSemaphoreSubmitInfo *signalSemaphoreInfo, VkSemaphoreSubmitInfo *waitSemaphoreInfo);
}  // namespace vkinit