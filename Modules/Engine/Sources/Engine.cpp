#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <VkGuide/Engine.hpp>
#include <VkGuide/VkInits.hpp>
#include <VkGuide/VkPipelines.hpp>
#include <VkGuide/VkUtils.hpp>
#include <VkGuide/VkCamera.hpp>

#include <chrono>
#include <thread>

#if defined(VKGUIDE_BUILD_TYPE_RELEASE)
constexpr bool g_UseValidationLayers{true};
#else
constexpr bool g_UseValidationLayers{true};
#endif

VulkanEngine VulkanEngine::g_VkEngine{};

VulkanEngine &VulkanEngine::GetInstance() {
    return g_VkEngine;
}

void VulkanEngine::init() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags windowFlags{(SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE)};

    m_Window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        m_WindowExtent.width,
        m_WindowExtent.height,
        windowFlags);

    initVulkan();
    initSwapchain();
    initCommands();
    initSyncStructures();
    initDescriptors();
    initPipelines();
    initImGui();
    initDefaultData();

    m_IsInitialized = true;
}

void VulkanEngine::cleanup() {
    if (!m_IsInitialized) return;

    vkDeviceWaitIdle(m_Device);

    for (FrameData &frame : m_Frames) {
        vkDestroyCommandPool(m_Device, frame.CommandPool, nullptr);
        vkDestroyFence(m_Device, frame.RenderFence, nullptr);
        vkDestroySemaphore(m_Device, frame.SwapchainSemaphore, nullptr);
        vkDestroySemaphore(m_Device, frame.RenderSemaphore, nullptr);
        frame.DeletionQueue.flush();
    }

    m_MainDeletionQueue.flush();

    destroySwapchain();

    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyDevice(m_Device, nullptr);

    vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
    vkDestroyInstance(m_Instance, nullptr);

    SDL_DestroyWindow(m_Window);
}

void VulkanEngine::draw() {
    FrameData &frame = getCurrentFrame();

    VK_CHECK(vkWaitForFences(m_Device, 1, &frame.RenderFence, VK_TRUE, 1000000000));
    frame.DeletionQueue.flush();
    VK_CHECK(vkResetFences(m_Device, 1, &frame.RenderFence));

    std::uint32_t swapchainImageIndex;
    {
        VkResult result = vkAcquireNextImageKHR(m_Device, m_Swapchain, 1000000000, frame.SwapchainSemaphore, nullptr, &swapchainImageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            m_ResizeRequested = true;
            return;
        }
    }
    const VkImage &swapchainImage = m_SwapchainImages[swapchainImageIndex];
    const VkImageView &swapchainImageView = m_SwapchainImageViews[swapchainImageIndex];

    m_DrawExtent = VkExtent2D{
        .width = (std::uint32_t)((float)std::min(m_SwapchainExtent.width, m_DrawImage.Extent.width) * m_RenderScale),
        .height = (std::uint32_t)((float)std::min(m_SwapchainExtent.height, m_DrawImage.Extent.height) * m_RenderScale),
    };

    const VkCommandBuffer &commandBuffer = frame.CommandBuffer;
    VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));
    VkCommandBufferBeginInfo commandBufferBeginInfo = vkinit::GetCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

    vkutils::TransitionImageLayout(commandBuffer, m_DrawImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    drawBackground(commandBuffer);
    vkutils::TransitionImageLayout(commandBuffer, m_DrawImage.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vkutils::TransitionImageLayout(commandBuffer, m_DepthImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    drawGeometry(commandBuffer);

    vkutils::TransitionImageLayout(commandBuffer, m_DrawImage.Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutils::TransitionImageLayout(commandBuffer, swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkutils::CopyImageToImage(commandBuffer, m_DrawImage.Image, swapchainImage, m_DrawExtent, m_SwapchainExtent);

    vkutils::TransitionImageLayout(commandBuffer, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    drawImGui(commandBuffer, swapchainImageView);

    vkutils::TransitionImageLayout(commandBuffer, swapchainImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    VkCommandBufferSubmitInfo commandBufferSubmitInfo = vkinit::GetCommandBufferSubmitInfo(commandBuffer);
    VkSemaphoreSubmitInfo waitInfo = vkinit::GetSemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame.SwapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = vkinit::GetSemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame.RenderSemaphore);
    VkSubmitInfo2 submitInfo = vkinit::GetSubmitInfo(commandBufferSubmitInfo, &signalInfo, &waitInfo);
    VK_CHECK(vkQueueSubmit2(m_GraphicsQueue, 1, &submitInfo, frame.RenderFence));

    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,

        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame.RenderSemaphore,

        .swapchainCount = 1,
        .pSwapchains = &m_Swapchain,

        .pImageIndices = &swapchainImageIndex,
    };

    {
        VkResult result = vkQueuePresentKHR(m_GraphicsQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            m_ResizeRequested = true;
            return;
        }
    }

    m_FrameNumber++;
}

void VulkanEngine::drawBackground(VkCommandBuffer commandBuffer) {
    VkClearColorValue clearValue{{0.0f, 0.0f, std::abs(std::sin(m_FrameNumber / 120.0f)), 1.0f}};
    VkImageSubresourceRange clearRange = vkinit::GetImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(commandBuffer, m_DrawImage.Image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

    ComputeEffect &effect = m_BackgroundEffects[m_CurrentBackgroundEffect];

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, effect.Pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, effect.Layout, 0, 1, &m_DrawImageDescriptors, 0, nullptr);
    vkCmdPushConstants(commandBuffer, m_GradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.Data);
    vkCmdDispatch(commandBuffer, std::ceil(m_DrawExtent.width / 16.0), std::ceil(m_DrawExtent.height / 16.0), 1);
}

void VulkanEngine::drawImGui(VkCommandBuffer commandBuffer, VkImageView targetImageView) {
    VkRenderingAttachmentInfo colorAttachmentInfo = vkinit::GetAttachmentInfo(targetImageView, nullptr);
    VkRenderingInfo renderInfo = vkinit::GetRenderingInfo(m_SwapchainExtent, colorAttachmentInfo, nullptr);

    vkCmdBeginRendering(commandBuffer, &renderInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    vkCmdEndRendering(commandBuffer);
}

void VulkanEngine::drawGeometry(VkCommandBuffer commandBuffer) {
    VkViewport viewport{
        .x = 0,
        .y = 0,
        .width = (float)m_DrawExtent.width,
        .height = (float)m_DrawExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor{
        .offset = VkOffset2D{.x = 0, .y = 0},
        .extent = m_DrawExtent,
    };

    VkRenderingAttachmentInfo colorAttachment = vkinit::GetAttachmentInfo(m_DrawImage.View, nullptr);
    VkRenderingAttachmentInfo depthAttachment = vkinit::GetDepthAttachmentInfo(m_DepthImage.View);

    VkRenderingInfo renderInfo = vkinit::GetRenderingInfo(m_DrawExtent, colorAttachment, &depthAttachment);
    vkCmdBeginRendering(commandBuffer, &renderInfo);

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_TrianglePipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MeshPipeline);
    static GPUDrawPushConstants pushConstants{};
    pushConstants.WorldMatrix = glm::mat4{1.0f};
    pushConstants.VertexBuffer = m_Rectangle.VertexBufferAddress;
    vkCmdPushConstants(commandBuffer, m_MeshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &pushConstants);
    vkCmdBindIndexBuffer(commandBuffer, m_Rectangle.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

    glm::mat4 view = glm::translate(glm::vec3{0.0f, 0.0f, -5.0f});
    // glm::mat4 projection = glm::perspective(glm::radians(70.0f), (float)m_DrawExtent.width / (float)m_DrawExtent.height, 10000.0f, 0.1f);
    glm::mat4 projection = glm::perspective(glm::radians(70.0f), (float)m_DrawExtent.width / (float)m_DrawExtent.height, 0.1f, 10000.0f);
    projection[1][1] *= -1;
    pushConstants.WorldMatrix = projection * view;
    pushConstants.VertexBuffer = m_TestMeshes[2]->MeshBuffers.VertexBufferAddress;
    vkCmdPushConstants(commandBuffer, m_MeshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &pushConstants);
    vkCmdBindIndexBuffer(commandBuffer, m_TestMeshes[2]->MeshBuffers.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, m_TestMeshes[2]->Surfaces[0].Count, 1, m_TestMeshes[2]->Surfaces[0].StartIndex, 0, 0);

    vkCmdEndRendering(commandBuffer);
}

void VulkanEngine::immediateSubmit(std::function<void(VkCommandBuffer commandBuffer)> &&function) {
    VK_CHECK(vkResetFences(m_Device, 1, &m_ImmFence));
    VK_CHECK(vkResetCommandBuffer(m_ImmCommandBuffer, 0));
    const VkCommandBuffer &commandBuffer = m_ImmCommandBuffer;

    VkCommandBufferBeginInfo beginInfo = vkinit::GetCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    function(commandBuffer);

    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    VkCommandBufferSubmitInfo commandSubmitInfo = vkinit::GetCommandBufferSubmitInfo(commandBuffer);
    VkSubmitInfo2 submitInfo = vkinit::GetSubmitInfo(commandSubmitInfo, nullptr, nullptr);

    VK_CHECK(vkQueueSubmit2(m_GraphicsQueue, 1, &submitInfo, m_ImmFence));
    VK_CHECK(vkWaitForFences(m_Device, 1, &m_ImmFence, true, 9999999999));
}

void VulkanEngine::destroyBuffer(const AllocatedBuffer &buffer) {
    vmaDestroyBuffer(m_Allocator, buffer.Buffer, buffer.Allocation);
}

void VulkanEngine::run() {
    SDL_Event e{0};
    bool quit{false};

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }

            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED)
                    m_StopRendering = true;
                else if (e.window.event == SDL_WINDOWEVENT_MAXIMIZED)
                    m_StopRendering = false;
            }

            ImGui_ImplSDL2_ProcessEvent(&e);
        }

        if (m_StopRendering) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (m_ResizeRequested) {
            resizeSwapchain();
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("Background")) {
            ComputeEffect &selected = m_BackgroundEffects[m_CurrentBackgroundEffect];

            ImGui::SliderFloat("Render Scale", &m_RenderScale, 0.3f, 1.0f);

            ImGui::Text("Selected effect: ", selected.Name);

            ImGui::SliderInt("Effect Index", (int *)&m_CurrentBackgroundEffect, 0, m_BackgroundEffects.size() - 1);

            ImGui::InputFloat4("Data1", (float *)&selected.Data.Data1);
            ImGui::InputFloat4("Data2", (float *)&selected.Data.Data2);
            ImGui::InputFloat4("Data3", (float *)&selected.Data.Data3);
            ImGui::InputFloat4("Data4", (float *)&selected.Data.Data4);
        }
        ImGui::End();

        ImGui::Render();
        ImGui::EndFrame();

        draw();
    }
}

void VulkanEngine::initVulkan() {
    vkb::Result<vkb::Instance> vkbInstanceResult =
        vkb::InstanceBuilder{}
            .set_app_name("VkGuide Vulkan Application")
            .enable_validation_layers(g_UseValidationLayers)
            .request_validation_layers(g_UseValidationLayers)
            .use_default_debug_messenger()
            .require_api_version(1, 3, 0)
            .build();
    assert(vkbInstanceResult.has_value());
    vkb::Instance vkbInstance{vkbInstanceResult.value()};

    assert(SDL_Vulkan_CreateSurface(m_Window, vkbInstance, &m_Surface));

    vkb::Result<vkb::PhysicalDevice> vkbPhysicalDeviceResult =
        vkb::PhysicalDeviceSelector{vkbInstance}
            .set_minimum_version(1, 3)
            .set_required_features_12(VkPhysicalDeviceVulkan12Features{
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
                .descriptorIndexing = true,
                .bufferDeviceAddress = true,
            })
            .set_required_features_13(VkPhysicalDeviceVulkan13Features{
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
                .synchronization2 = true,
                .dynamicRendering = true,
            })
            .set_surface(m_Surface)
            .select();
    assert(vkbPhysicalDeviceResult.has_value());
    vkb::PhysicalDevice vkbPhysicalDevice{vkbPhysicalDeviceResult.value()};

    vkb::Result<vkb::Device> vkbDeviceResult =
        vkb::DeviceBuilder{vkbPhysicalDevice}
            .build();
    assert(vkbDeviceResult.has_value());
    vkb::Device vkbDevice{vkbDeviceResult.value()};
    vkb::Result<VkQueue> graphicsQueueResult = vkbDevice.get_queue(vkb::QueueType::graphics);
    vkb::Result<std::uint32_t> graphicsQueueIndexResult = vkbDevice.get_queue_index(vkb::QueueType::graphics);
    assert(graphicsQueueResult.has_value());
    assert(graphicsQueueIndexResult.has_value());

    m_Instance = vkbInstance;
    m_DebugMessenger = vkbInstance.debug_messenger;
    m_PhysicalDevice = vkbPhysicalDevice;
    m_Device = vkbDevice;
    m_GraphicsQueue = graphicsQueueResult.value();
    m_GraphicsQueueIndex = graphicsQueueIndexResult.value();

    VmaAllocatorCreateInfo allocatorInfo{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = m_PhysicalDevice,
        .device = m_Device,
        .instance = m_Instance,
    };
    vmaCreateAllocator(&allocatorInfo, &m_Allocator);

    m_MainDeletionQueue.pushFunction([this]() {
        vmaDestroyAllocator(m_Allocator);
    });
}

void VulkanEngine::initSwapchain() {
    createSwapchain(m_WindowExtent.width, m_WindowExtent.height);

    VkExtent3D imageExtent{
        .width = m_WindowExtent.width,
        .height = m_WindowExtent.height,
        .depth = 1,
    };

    VmaAllocationCreateInfo imageAllocationInfo{
        .flags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    };

    m_DrawImage.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
    m_DrawImage.Extent = imageExtent;
    VkImageUsageFlags drawImageUsages =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkImageCreateInfo rImageInfo = vkinit::GetImageCreateInfo(m_DrawImage.Format, drawImageUsages, imageExtent);
    vmaCreateImage(m_Allocator, &rImageInfo, &imageAllocationInfo, &m_DrawImage.Image, &m_DrawImage.Allocation, nullptr);
    VkImageViewCreateInfo rImageViewInfo = vkinit::GetImageViewCreateInfo(m_DrawImage.Format, m_DrawImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(m_Device, &rImageViewInfo, nullptr, &m_DrawImage.View));

    m_DepthImage.Extent = imageExtent;
    m_DepthImage.Format = VK_FORMAT_D32_SFLOAT;
    VkImageUsageFlags depthImageUsages{VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT};
    VkImageCreateInfo dImageInfo = vkinit::GetImageCreateInfo(m_DepthImage.Format, depthImageUsages, imageExtent);
    vmaCreateImage(m_Allocator, &dImageInfo, &imageAllocationInfo, &m_DepthImage.Image, &m_DepthImage.Allocation, nullptr);
    VkImageViewCreateInfo dImageViewInfo = vkinit::GetImageViewCreateInfo(m_DepthImage.Format, m_DepthImage.Image, VK_IMAGE_ASPECT_DEPTH_BIT);
    VK_CHECK(vkCreateImageView(m_Device, &dImageViewInfo, nullptr, &m_DepthImage.View));

    m_MainDeletionQueue.pushFunction([this]() {
        vkDestroyImageView(m_Device, m_DrawImage.View, nullptr);
        vkDestroyImageView(m_Device, m_DepthImage.View, nullptr);
        vmaDestroyImage(m_Allocator, m_DrawImage.Image, m_DrawImage.Allocation);
        vmaDestroyImage(m_Allocator, m_DepthImage.Image, m_DepthImage.Allocation);
    });
}

void VulkanEngine::initCommands() {
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::GetCommandPoolCreateInfo(m_GraphicsQueueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (std::uint32_t i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_Frames[i].CommandPool));

        VkCommandBufferAllocateInfo commandAllocateInfo = vkinit::GetCommandBufferAllocateInfo(m_Frames[i].CommandPool);

        VK_CHECK(vkAllocateCommandBuffers(m_Device, &commandAllocateInfo, &m_Frames[i].CommandBuffer));
    }

    {
        VK_CHECK(vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_ImmCommandPool));
        VkCommandBufferAllocateInfo commandAllocateInfo = vkinit::GetCommandBufferAllocateInfo(m_ImmCommandPool);
        VK_CHECK(vkAllocateCommandBuffers(m_Device, &commandAllocateInfo, &m_ImmCommandBuffer));

        m_MainDeletionQueue.pushFunction([this]() {
            vkDestroyCommandPool(m_Device, m_ImmCommandPool, nullptr);
        });
    }
}

void VulkanEngine::initSyncStructures() {
    VkFenceCreateInfo fenceCreateInfo = vkinit::GetFenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::GetSemaphoreCreateInfo();

    for (std::uint32_t i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_Frames[i].RenderFence));

        VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].SwapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].RenderSemaphore));
    }

    VK_CHECK(vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_ImmFence));
    m_MainDeletionQueue.pushFunction([this]() {
        vkDestroyFence(m_Device, m_ImmFence, nullptr);
    });
}

void VulkanEngine::initDescriptors() {
    std::vector<DescriptorAllocator::PoolSizeRatio> sizeRatios{DescriptorAllocator::PoolSizeRatio{
        .Type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .Ratio = 1,
    }};

    m_GlobalDescriptorAllocator.initPool(m_Device, 10, sizeRatios);

    {
        DescriptorLayoutBuilder builder{};
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        m_DrawImageDescriptorLayout = builder.build(m_Device, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    m_DrawImageDescriptors = m_GlobalDescriptorAllocator.allocate(m_Device, m_DrawImageDescriptorLayout);

    VkDescriptorImageInfo imageInfo{
        .imageView = m_DrawImage.View,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };

    VkWriteDescriptorSet drawImageWrite{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_DrawImageDescriptors,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &imageInfo,
    };

    vkUpdateDescriptorSets(m_Device, 1, &drawImageWrite, 0, nullptr);

    m_MainDeletionQueue.pushFunction([this]() {
        m_GlobalDescriptorAllocator.destroyPool(m_Device);
        vkDestroyDescriptorSetLayout(m_Device, m_DrawImageDescriptorLayout, nullptr);
    });
}

void VulkanEngine::initPipelines() {
    initBackgroundPipelines();
    initTrianglePipeline();
    initMeshPipeline();
}

void VulkanEngine::initBackgroundPipelines() {
    VkPushConstantRange pushConstant{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(ComputePushConstants),
    };
    VkPipelineLayoutCreateInfo computeLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_DrawImageDescriptorLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstant,
    };
    VK_CHECK(vkCreatePipelineLayout(m_Device, &computeLayoutInfo, nullptr, &m_GradientPipelineLayout));

    VkShaderModule gradientShaderModule{VK_NULL_HANDLE};
    VkShaderModule skyShaderModule{VK_NULL_HANDLE};
    assert(vkutils::LoadShaderModule(m_Device, "Assets/Shaders/GradientColor.comp.spv", &gradientShaderModule));
    assert(vkutils::LoadShaderModule(m_Device, "Assets/Shaders/Sky.comp.spv", &skyShaderModule));

    VkComputePipelineCreateInfo computePipelineInfo{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = gradientShaderModule,
            .pName = "main",
        },
        .layout = m_GradientPipelineLayout,
    };

    ComputeEffect gradient{
        .Name = "Gradient",
        .Layout = m_GradientPipelineLayout,
        .Data = ComputePushConstants{
            .Data1 = glm::vec4{1.0f, 0.0f, 0.0f, 1.0f},
            .Data2 = glm::vec4{0.0f, 0.0f, 1.0f, 1.0f},
        },
    };

    VK_CHECK(vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &gradient.Pipeline));

    computePipelineInfo.stage.module = skyShaderModule;

    ComputeEffect sky{
        .Name = "Sky",
        .Layout = m_GradientPipelineLayout,
        .Data = ComputePushConstants{.Data1 = glm::vec4{0.1, 0.2, 0.4, 0.97}},
    };

    VK_CHECK(vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &sky.Pipeline));

    m_BackgroundEffects.emplace_back(gradient);
    m_BackgroundEffects.emplace_back(sky);

    vkDestroyShaderModule(m_Device, gradientShaderModule, nullptr);
    vkDestroyShaderModule(m_Device, skyShaderModule, nullptr);

    m_MainDeletionQueue.pushFunction([this]() {
        for (const ComputeEffect &computeEffect : m_BackgroundEffects) {
            vkDestroyPipeline(m_Device, computeEffect.Pipeline, nullptr);
        }
        vkDestroyPipelineLayout(m_Device, m_GradientPipelineLayout, nullptr);
    });
}

void VulkanEngine::initTrianglePipeline() {
    VkShaderModule triangleVertShader{VK_NULL_HANDLE};
    VkShaderModule triangleFragShader{VK_NULL_HANDLE};
    assert(vkutils::LoadShaderModule(m_Device, "Assets/Shaders/ColoredTriangle.vert.spv", &triangleVertShader));
    assert(vkutils::LoadShaderModule(m_Device, "Assets/Shaders/ColoredTriangle.frag.spv", &triangleFragShader));

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::GetPipelineLayoutInfo();
    VK_CHECK(vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_TrianglePipelineLayout));

    PipelineBuilder pipelineBuilder{};
    pipelineBuilder.setShaders(triangleVertShader, triangleFragShader);
    pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipelineBuilder.setMultisamplingNone();
    pipelineBuilder.setBlendingDisabled();
    pipelineBuilder.setDepthTestDisabled();
    pipelineBuilder.setColorAttachmentFormat(m_DrawImage.Format);
    pipelineBuilder.setDepthFormat(m_DepthImage.Format);

    m_TrianglePipeline = pipelineBuilder.build(m_Device, m_TrianglePipelineLayout);

    vkDestroyShaderModule(m_Device, triangleVertShader, nullptr);
    vkDestroyShaderModule(m_Device, triangleFragShader, nullptr);

    m_MainDeletionQueue.pushFunction([this]() {
        vkDestroyPipeline(m_Device, m_TrianglePipeline, nullptr);
        vkDestroyPipelineLayout(m_Device, m_TrianglePipelineLayout, nullptr);
    });
}

void VulkanEngine::initMeshPipeline() {
    VkShaderModule triangleVertShader{VK_NULL_HANDLE};
    VkShaderModule triangleFragShader{VK_NULL_HANDLE};
    assert(vkutils::LoadShaderModule(m_Device, "Assets/Shaders/ColoredTriangleMesh.vert.spv", &triangleVertShader));
    assert(vkutils::LoadShaderModule(m_Device, "Assets/Shaders/ColoredTriangle.frag.spv", &triangleFragShader));

    VkPushConstantRange bufferRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(GPUDrawPushConstants),
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::GetPipelineLayoutInfo();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &bufferRange;
    VK_CHECK(vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_MeshPipelineLayout));

    PipelineBuilder pipelineBuilder{};
    pipelineBuilder.setShaders(triangleVertShader, triangleFragShader);
    pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipelineBuilder.setMultisamplingNone();
    pipelineBuilder.setBlendingAdditiveEnabled();
    pipelineBuilder.setDepthTestEnabled(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineBuilder.setColorAttachmentFormat(m_DrawImage.Format);
    pipelineBuilder.setDepthFormat(m_DepthImage.Format);

    m_MeshPipeline = pipelineBuilder.build(m_Device, m_MeshPipelineLayout);

    vkDestroyShaderModule(m_Device, triangleVertShader, nullptr);
    vkDestroyShaderModule(m_Device, triangleFragShader, nullptr);

    m_MainDeletionQueue.pushFunction([this]() {
        vkDestroyPipeline(m_Device, m_MeshPipeline, nullptr);
        vkDestroyPipelineLayout(m_Device, m_MeshPipelineLayout, nullptr);
    });
}

void VulkanEngine::initImGui() {
    std::vector<VkDescriptorPoolSize> poolSizes{
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = 1000},
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1000},
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = 1000},
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1000},
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, .descriptorCount = 1000},
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, .descriptorCount = 1000},
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1000},
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1000},
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, .descriptorCount = 1000},
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, .descriptorCount = 1000},
        VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, .descriptorCount = 1000},
    };

    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1000,
        .poolSizeCount = (std::uint32_t)poolSizes.size(),
        .pPoolSizes = poolSizes.data(),
    };

    VkDescriptorPool imguiPool{VK_NULL_HANDLE};
    VK_CHECK(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &imguiPool));

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForVulkan(m_Window);

    ImGui_ImplVulkan_InitInfo initInfo{
        .Instance = m_Instance,
        .PhysicalDevice = m_PhysicalDevice,
        .Device = m_Device,
        .Queue = m_GraphicsQueue,
        .DescriptorPool = imguiPool,
        .MinImageCount = 3,
        .ImageCount = 3,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo = VkPipelineRenderingCreateInfoKHR{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &m_SwapchainImageFormat,
        },
    };

    ImGui_ImplVulkan_Init(&initInfo);
    ImGui_ImplVulkan_CreateFontsTexture();

    m_MainDeletionQueue.pushFunction([imguiPool, this]() {
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(m_Device, imguiPool, nullptr);
    });
}

void VulkanEngine::initDefaultData() {
    std::array<Vertex, 4> rectVertices{
        Vertex{.Position = glm::vec3{0.5f, -0.5f, 0.0f}, .Color = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}},
        Vertex{.Position = glm::vec3{0.5f, 0.5f, 0.0f}, .Color = glm::vec4{0.5f, 0.5f, 0.5f, 1.0f}},
        Vertex{.Position = glm::vec3{-0.5f, -0.5f, 0.0f}, .Color = glm::vec4{1.0f, 0.0f, 0.0f, 1.0f}},
        Vertex{.Position = glm::vec3{-0.5f, 0.5f, 0.0f}, .Color = glm::vec4{0.0f, 1.0f, 0.0f, 1.0f}},
    };

    std::array<std::uint32_t, 6> rectIndices{0, 1, 2, 2, 1, 3};

    m_Rectangle = createMesh(rectIndices, rectVertices);

    m_TestMeshes = loadGltfMeshes(this, "Assets/Models/basicmesh.glb").value();

    m_MainDeletionQueue.pushFunction([this]() {
        destroyBuffer(m_Rectangle.IndexBuffer);
        destroyBuffer(m_Rectangle.VertexBuffer);

        for (const std::shared_ptr<MeshAsset> &mesh : m_TestMeshes) {
            destroyBuffer(mesh->MeshBuffers.IndexBuffer);
            destroyBuffer(mesh->MeshBuffers.VertexBuffer);
        }
    });
}

void VulkanEngine::createSwapchain(std::uint32_t width, std::uint32_t height) {
    if (m_Swapchain != VK_NULL_HANDLE) {
        destroySwapchain();
    }

    m_SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Result<vkb::Swapchain> vkbSwapchainResult =
        vkb::SwapchainBuilder{m_PhysicalDevice, m_Device, m_Surface}
            .set_desired_format(VkSurfaceFormatKHR{
                .format = m_SwapchainImageFormat,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            })
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(width, height)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build();
    assert(vkbSwapchainResult.has_value());
    vkb::Swapchain vkbSwapchain{vkbSwapchainResult.value()};
    vkb::Result<std::vector<VkImage>> vkbSwapchainImagesResult{vkbSwapchain.get_images()};
    vkb::Result<std::vector<VkImageView>> vkbSwapchainImageViewsResult{vkbSwapchain.get_image_views()};
    assert(vkbSwapchainImagesResult.has_value());
    assert(vkbSwapchainImageViewsResult.has_value());

    m_SwapchainExtent = vkbSwapchain.extent;
    m_Swapchain = vkbSwapchain;
    m_SwapchainImages = vkbSwapchainImagesResult.value();
    m_SwapchainImageViews = vkbSwapchainImageViewsResult.value();
}

void VulkanEngine::destroySwapchain() {
    if (m_Swapchain == VK_NULL_HANDLE) return;

    for (VkImageView &view : m_SwapchainImageViews) {
        vkDestroyImageView(m_Device, view, nullptr);
    }
    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
}

void VulkanEngine::resizeSwapchain() {
    vkDeviceWaitIdle(m_Device);

    std::int32_t width{0};
    std::int32_t height{0};
    SDL_GetWindowSize(m_Window, &width, &height);

    createSwapchain(width, height);

    m_ResizeRequested = false;
}

FrameData &VulkanEngine::getCurrentFrame() {
    return m_Frames[m_FrameNumber % FRAME_OVERLAP];
}

AllocatedBuffer VulkanEngine::createBuffer(std::size_t allocationSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = allocationSize,
        .usage = usage,
    };

    VmaAllocationCreateInfo vmaAllocationInfo{
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = memoryUsage,
    };

    AllocatedBuffer buffer{};
    VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &vmaAllocationInfo, &buffer.Buffer, &buffer.Allocation, &buffer.Info));
    return buffer;
}

GPUMeshBuffers VulkanEngine::createMesh(const std::span<std::uint32_t> &indices, const std::span<Vertex> &vertices) {
    const std::size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    const std::size_t indexBufferSize = indices.size() * sizeof(std::uint32_t);

    GPUMeshBuffers surface{};
    surface.VertexBuffer = createBuffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    VkBufferDeviceAddressInfo deviceAddressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = surface.VertexBuffer.Buffer,
    };
    surface.VertexBufferAddress = vkGetBufferDeviceAddress(m_Device, &deviceAddressInfo);
    surface.IndexBuffer = createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

    AllocatedBuffer staging = createBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void *data = staging.Allocation->GetMappedData();
    memcpy(data, vertices.data(), vertexBufferSize);
    memcpy((char *)data + vertexBufferSize, indices.data(), indexBufferSize);

    immediateSubmit([&](VkCommandBuffer commandBuffer) {
        VkBufferCopy vertexCopy{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = vertexBufferSize,
        };
        VkBufferCopy indexCopy{
            .srcOffset = vertexBufferSize,
            .dstOffset = 0,
            .size = indexBufferSize,
        };

        vkCmdCopyBuffer(commandBuffer, staging.Buffer, surface.VertexBuffer.Buffer, 1, &vertexCopy);
        vkCmdCopyBuffer(commandBuffer, staging.Buffer, surface.IndexBuffer.Buffer, 1, &indexCopy);
    });

    destroyBuffer(staging);

    return surface;
}