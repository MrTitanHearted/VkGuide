#include <VkGuide/VkPipelines.hpp>
#include <VkGuide/VkInits.hpp>

#include <fstream>

namespace vkutils {
    bool LoadShaderModule(VkDevice device, const char* filePath, VkShaderModule* outShaderModule) {
        std::ifstream file{filePath, std::ios::ate | std::ios::binary};
        if (!file.is_open()) {
            fmt::println("[ERROR]: Failed to open file from path: {}.", filePath);
            return false;
        }

        std::size_t fileSize = file.tellg();

        std::vector<std::uint32_t> buffer{};
        buffer.reserve(fileSize / sizeof(std::uint32_t));

        file.seekg(0);
        file.read((char*)buffer.data(), fileSize);
        file.close();

        VkShaderModuleCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = fileSize,
            .pCode = buffer.data(),
        };

        if (vkCreateShaderModule(device, &info, nullptr, outShaderModule) != VK_SUCCESS) {
            return false;
        }
        return true;
    }
}  // namespace vkutils