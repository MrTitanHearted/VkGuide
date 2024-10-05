#include <VkGuide/VkGuide.hpp>

int main(int argc, char **argv) {
    VulkanEngine &vkEngine = VulkanEngine::GetInstance();

    vkEngine.init();
    vkEngine.run();
    vkEngine.cleanup();

    return 0;
}