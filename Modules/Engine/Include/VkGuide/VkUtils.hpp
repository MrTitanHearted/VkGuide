#pragma once

#include <VkGuide/Defines.hpp>

namespace vkutils {
    void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

    void CopyImageToImage(VkCommandBuffer commandBuffer, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize);
}  // namespace vkutils