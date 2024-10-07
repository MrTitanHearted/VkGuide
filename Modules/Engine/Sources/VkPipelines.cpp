#include <VkGuide/VkInits.hpp>
#include <VkGuide/VkPipelines.hpp>
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

PipelineBuilder::PipelineBuilder() {
    clear();
}

void PipelineBuilder::setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader) {
    m_ShaderStages.clear();
    m_ShaderStages.emplace_back(vkinit::GetPipelineShaderStageInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
    m_ShaderStages.emplace_back(vkinit::GetPipelineShaderStageInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
}

void PipelineBuilder::setInputTopology(VkPrimitiveTopology topology) {
    m_InputAssembly.topology = topology;
    m_InputAssembly.primitiveRestartEnable = VK_FALSE;
}

void PipelineBuilder::setPolygonMode(VkPolygonMode mode) {
    m_Rasterization.polygonMode = mode;
    m_Rasterization.lineWidth = 1.0f;
}

void PipelineBuilder::setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace) {
    m_Rasterization.cullMode = cullMode;
    m_Rasterization.frontFace = frontFace;
}

void PipelineBuilder::setColorAttachmentFormat(VkFormat format) {
    m_ColorAttachmentFormat = format;
    m_RenderInfo.colorAttachmentCount = 1;
    m_RenderInfo.pColorAttachmentFormats = &m_ColorAttachmentFormat;
}

void PipelineBuilder::setDepthFormat(VkFormat format) {
    m_RenderInfo.depthAttachmentFormat = format;
}

void PipelineBuilder::setDepthTestEnabled(bool depthWriteEnable, VkCompareOp op) {
    m_DepthStencil.depthTestEnable = VK_TRUE;
    m_DepthStencil.depthWriteEnable = depthWriteEnable ? VK_TRUE : VK_FALSE;
    m_DepthStencil.depthCompareOp = op;
    m_DepthStencil.depthBoundsTestEnable = VK_FALSE;
    m_DepthStencil.stencilTestEnable = VK_FALSE;
    m_DepthStencil.front = VkStencilOpState{};
    m_DepthStencil.back = VkStencilOpState{};
    m_DepthStencil.minDepthBounds = 0.0f;
    m_DepthStencil.maxDepthBounds = 1.0f;
}

void PipelineBuilder::setMultisamplingNone() {
    m_Multisampling.sampleShadingEnable = VK_FALSE;
    m_Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    m_Multisampling.minSampleShading = 1.0f;
    m_Multisampling.alphaToCoverageEnable = VK_FALSE;
    m_Multisampling.alphaToOneEnable = VK_FALSE;
}

void PipelineBuilder::setBlendingDisabled() {
    m_ColorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    m_ColorBlendAttachment.blendEnable = VK_FALSE;
}

void PipelineBuilder::setDepthTestDisabled() {
    m_DepthStencil.depthTestEnable = VK_FALSE;
    m_DepthStencil.depthWriteEnable = VK_FALSE;
    m_DepthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    m_DepthStencil.depthBoundsTestEnable = VK_FALSE;
    m_DepthStencil.stencilTestEnable = VK_FALSE;
    m_DepthStencil.front = VkStencilOpState{};
    m_DepthStencil.back = VkStencilOpState{};
    m_DepthStencil.minDepthBounds = 0.0f;
    m_DepthStencil.maxDepthBounds = 1.0f;
}

void PipelineBuilder::clear() {
    m_InputAssembly = VkPipelineInputAssemblyStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };
    m_Rasterization = VkPipelineRasterizationStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    };
    m_ColorBlendAttachment = VkPipelineColorBlendAttachmentState{};
    m_Multisampling = VkPipelineMultisampleStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    };
    m_DepthStencil = VkPipelineDepthStencilStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    };
    m_RenderInfo = VkPipelineRenderingCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    };
    m_ShaderStages.clear();
}

VkPipeline PipelineBuilder::build(VkDevice device, VkPipelineLayout layout) {
    assert(m_ColorAttachmentFormat != VK_FORMAT_UNDEFINED);

    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &m_ColorBlendAttachment,
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    VkDynamicState dynamicStates[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState),
        .pDynamicStates = dynamicStates,
    };

    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &m_RenderInfo,
        .stageCount = (std::uint32_t)m_ShaderStages.size(),
        .pStages = m_ShaderStages.data(),
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &m_InputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &m_Rasterization,
        .pMultisampleState = &m_Multisampling,
        .pDepthStencilState = &m_DepthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicInfo,
        .layout = layout,
    };

    VkPipeline pipeline{VK_NULL_HANDLE};
    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
    return pipeline;
}