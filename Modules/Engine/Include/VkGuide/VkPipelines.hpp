#pragma once

#include <VkGuide/Defines.hpp>

namespace vkutils {
    bool LoadShaderModule(VkDevice device, const char *filePath, VkShaderModule *outShaderModule);
}  // namespace vkutils
