#include <VkGuide/VkDescriptors.hpp>

void DescriptorLayoutBuilder::addBinding(std::uint32_t binding, VkDescriptorType type) {
    m_Bindings.emplace_back(VkDescriptorSetLayoutBinding{
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = 1,
    });
}

void DescriptorLayoutBuilder::clear() {
    m_Bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::build(
    VkDevice device,
    VkShaderStageFlags shaderStages,
    void *pNext,
    VkDescriptorSetLayoutCreateFlags flags) {
    for (VkDescriptorSetLayoutBinding &binding : m_Bindings) {
        binding.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = pNext,
        .flags = flags,
        .bindingCount = (std::uint32_t)m_Bindings.size(),
        .pBindings = m_Bindings.data(),
    };

    VkDescriptorSetLayout setLayout{VK_NULL_HANDLE};
    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &setLayout));
    return setLayout;
}

void DescriptorAllocator::initPool(VkDevice device, std::uint32_t maxSets, const std::span<PoolSizeRatio> &poolRatios) {
    assert(m_Pool == VK_NULL_HANDLE);

    std::vector<VkDescriptorPoolSize> poolSizes{};
    poolSizes.reserve(poolRatios.size());
    for (const PoolSizeRatio &ratio : poolRatios) {
        poolSizes.emplace_back(VkDescriptorPoolSize{
            .type = ratio.Type,
            .descriptorCount = std::uint32_t(ratio.Ratio * maxSets),
        });
    }

    VkDescriptorPoolCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = maxSets,
        .poolSizeCount = (std::uint32_t)poolSizes.size(),
        .pPoolSizes = poolSizes.data(),
    };

    vkCreateDescriptorPool(device, &info, nullptr, &m_Pool);
}

void DescriptorAllocator::clearDescriptors(VkDevice device) {
    assert(m_Pool != VK_NULL_HANDLE);
    vkResetDescriptorPool(device, m_Pool, 0);
}

void DescriptorAllocator::destroyPool(VkDevice device) {
    assert(m_Pool != VK_NULL_HANDLE);
    vkDestroyDescriptorPool(device, m_Pool, nullptr);
    m_Pool = VK_NULL_HANDLE;
}

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout) {
    assert(m_Pool != VK_NULL_HANDLE);
    VkDescriptorSetAllocateInfo info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_Pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    VkDescriptorSet set{VK_NULL_HANDLE};
    VK_CHECK(vkAllocateDescriptorSets(device, &info, &set));
    return set;
}