#pragma once

#include <VkGuide/Defines.hpp>

struct AllocatedImage {
    VkImage Image;
    VkImageView View;
    VmaAllocation Allocation;
    VkExtent3D Extent;
    VkFormat Format;
};