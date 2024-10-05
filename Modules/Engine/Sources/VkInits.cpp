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

    VkSubmitInfo2 GetSubmitInfo(const VkCommandBufferSubmitInfo &commandBufferInfo, VkSemaphoreSubmitInfo *signalSemaphoreInfo, VkSemaphoreSubmitInfo *waitSemaphoreInfo) {
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
}  // namespace vkinit