#pragma once

#include <VkGuide/Defines.hpp>

class DescriptorLayoutBuilder {
   public:
    DescriptorLayoutBuilder() = default;
    ~DescriptorLayoutBuilder() = default;

    void addBinding(std::uint32_t binding, VkDescriptorType type);
    void clear();

    VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);

   private:
    std::vector<VkDescriptorSetLayoutBinding> m_Bindings;
};

class DescriptorAllocator {
   public:
    struct PoolSizeRatio {
        VkDescriptorType Type;
        float Ratio;
    };

   public:
    DescriptorAllocator() = default;
    ~DescriptorAllocator() = default;

    void initPool(VkDevice device, std::uint32_t maxSets, const std::span<PoolSizeRatio> &poolRatios);
    void clearDescriptors(VkDevice device);
    void destroyPool(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);

   private:
    VkDescriptorPool m_Pool{VK_NULL_HANDLE};
};