#pragma once

#include <VkGuide/Defines.hpp>

namespace vkinit {
    VkCommandPoolCreateInfo GetCommandPoolCreateInfo(std::uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
    VkCommandBufferAllocateInfo GetCommandBufferAllocateInfo(VkCommandPool pool, std::uint32_t count = 1);
}  // namespace vkinit