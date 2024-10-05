#pragma once

#include <VkGuide/Defines.hpp>

namespace vkutils {
    void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
}  // namespace vkutils