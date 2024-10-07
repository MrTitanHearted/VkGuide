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

    VkImageCreateInfo GetImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
    VkImageViewCreateInfo GetImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectMask);

    VkRenderingAttachmentInfo GetAttachmentInfo(VkImageView view, VkClearValue *clear, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo GetDepthAttachmentInfo(VkImageView view, VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    VkRenderingInfo GetRenderingInfo(VkExtent2D extent, const VkRenderingAttachmentInfo &colorAttachment, VkRenderingAttachmentInfo *depthAttachment);

    VkPipelineShaderStageCreateInfo GetPipelineShaderStageInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char *entry = "main");
    VkPipelineLayoutCreateInfo GetPipelineLayoutInfo();
}  // namespace vkinit
