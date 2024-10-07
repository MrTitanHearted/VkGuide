#pragma once

#include <VkGuide/Defines.hpp>

namespace vkutils {
    bool LoadShaderModule(VkDevice device, const char *filePath, VkShaderModule *outShaderModule);
}  // namespace vkutils

class PipelineBuilder {
   public:
    PipelineBuilder();
    ~PipelineBuilder() = default;

    void setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void setInputTopology(VkPrimitiveTopology topology);
    void setPolygonMode(VkPolygonMode mode);
    void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void setColorAttachmentFormat(VkFormat format);
    void setDepthFormat(VkFormat format);
    void setDepthTestEnabled(bool depthWriteEnable, VkCompareOp op);
    void setMultisamplingNone();
    void setBlendingDisabled();
    void setDepthTestDisabled();

    void clear();

    VkPipeline build(VkDevice device, VkPipelineLayout layout);
   private:
    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages{};
    VkPipelineInputAssemblyStateCreateInfo m_InputAssembly{};
    VkPipelineRasterizationStateCreateInfo m_Rasterization{};
    VkPipelineColorBlendAttachmentState m_ColorBlendAttachment{};
    VkPipelineMultisampleStateCreateInfo m_Multisampling{};
    VkPipelineDepthStencilStateCreateInfo m_DepthStencil{};
    VkPipelineRenderingCreateInfo m_RenderInfo{};
    VkFormat m_ColorAttachmentFormat{VK_FORMAT_UNDEFINED};
};