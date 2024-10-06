#include <VkGuide/VkInits.hpp>

namespace vkinit {
    VkCommandPoolCreateInfo GetCommandPoolCreateInfo(std::uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
        return VkCommandPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = queueFamilyIndex,
        };
    }

    VkCommandBufferAllocateInfo GetCommandBufferAllocateInfo(VkCommandPool pool, std::uint32_t count) {
        return VkCommandBufferAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
    }

    VkFenceCreateInfo GetFenceCreateInfo(VkFenceCreateFlags flags) {
        return VkFenceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = flags,
        };
    }

    VkSemaphoreCreateInfo GetSemaphoreCreateInfo(VkSemaphoreCreateFlags flags) {
        return VkSemaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .flags = flags,
        };
    }

    VkCommandBufferBeginInfo GetCommandBufferBeginInfo(VkCommandBufferUsageFlags flags) {
        return VkCommandBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = flags,
            .pInheritanceInfo = nullptr,
        };
    }

    VkImageSubresourceRange GetImageSubresourceRange(VkImageAspectFlags aspectMask) {
        return VkImageSubresourceRange{
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS,
        };
    }

    VkSemaphoreSubmitInfo GetSemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore) {
        return VkSemaphoreSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = semaphore,
            .value = 1,
            .stageMask = stageMask,
            .deviceIndex = 0,
        };
    }

    VkCommandBufferSubmitInfo GetCommandBufferSubmitInfo(VkCommandBuffer commandBuffer) {
        return VkCommandBufferSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = commandBuffer,
            .deviceMask = 0,
        };
    }

    VkSubmitInfo2 GetSubmitInfo(const VkCommandBufferSubmitInfo& commandBufferInfo, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo) {
        return VkSubmitInfo2{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,

            .waitSemaphoreInfoCount = (waitSemaphoreInfo == nullptr) ? 0U : 1U,
            .pWaitSemaphoreInfos = waitSemaphoreInfo,

            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBufferInfo,

            .signalSemaphoreInfoCount = (signalSemaphoreInfo == nullptr) ? 0U : 1U,
            .pSignalSemaphoreInfos = signalSemaphoreInfo,
        };
    }

    VkImageCreateInfo GetImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent) {
        return VkImageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .extent = extent,
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = usageFlags,
        };
    }

    VkImageViewCreateInfo GetImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectMask) {
        return VkImageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange = VkImageSubresourceRange{
                .aspectMask = aspectMask,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
    }

    VkRenderingAttachmentInfo GetAttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout) {
        VkRenderingAttachmentInfo colorAttachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = view,
            .imageLayout = layout,
            .loadOp = clear == nullptr ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        };

        if (clear != nullptr) {
            colorAttachment.clearValue = *clear;
        }

        return colorAttachment;
    }

    VkRenderingInfo GetRenderingInfo(VkExtent2D extent, const VkRenderingAttachmentInfo &colorAttachment, VkRenderingAttachmentInfo* depthAttachment) {
        return VkRenderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = VkRect2D{
                .offset = VkOffset2D{.x = 0, .y = 0},
                .extent = extent,
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment,
            .pDepthAttachment = depthAttachment,
            .pStencilAttachment = nullptr,
        };
    }
}  // namespace vkinit
