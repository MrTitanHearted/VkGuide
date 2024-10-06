#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <VkBootstrap.h>

#include <VkGuide/Engine.hpp>
#include <VkGuide/VkInits.hpp>
#include <VkGuide/VkPipelines.hpp>
#include <VkGuide/VkUtils.hpp>
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
    SDL_WindowFlags windowFlags{SDL_WINDOW_VULKAN};

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
    VK_CHECK(vkAcquireNextImageKHR(m_Device, m_Swapchain, 1000000000, frame.SwapchainSemaphore, nullptr, &swapchainImageIndex));
    const VkImage &swapchainImage = m_SwapchainImages[swapchainImageIndex];
    // const VkImageView &swapchainImageView = m_SwapchainImageViews[swapchainImageIndex];

    m_DrawExtent = VkExtent2D{
        .width = m_DrawImage.Extent.width,
        .height = m_DrawImage.Extent.height,
    };

    const VkCommandBuffer &commandBuffer = frame.CommandBuffer;
    VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));
    VkCommandBufferBeginInfo commandBufferBeginInfo = vkinit::GetCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

    vkutils::TransitionImageLayout(commandBuffer, m_DrawImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    drawBackground(commandBuffer);

    vkutils::TransitionImageLayout(commandBuffer, m_DrawImage.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutils::TransitionImageLayout(commandBuffer, swapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkutils::CopyImageToImage(commandBuffer, m_DrawImage.Image, swapchainImage, m_DrawExtent, m_SwapchainExtent);

    vkutils::TransitionImageLayout(commandBuffer, swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
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
    VK_CHECK(vkQueuePresentKHR(m_GraphicsQueue, &presentInfo));

    m_FrameNumber++;
}

void VulkanEngine::drawBackground(const VkCommandBuffer &commandBuffer) {
    VkClearColorValue clearValue{{0.0f, 0.0f, std::abs(std::sin(m_FrameNumber / 120.0f)), 1.0f}};
    VkImageSubresourceRange clearRange = vkinit::GetImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(commandBuffer, m_DrawImage.Image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_GradientPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_GradientPipelineLayout, 0, 1, &m_DrawImageDescriptors, 0, nullptr);
    vkCmdDispatch(commandBuffer, std::ceil(m_DrawExtent.width / 16.0), std::ceil(m_DrawExtent.height / 16.0), 1);
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
        }

        if (m_StopRendering) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

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

    VkExtent3D drawImageExtent{
        .width = m_WindowExtent.width,
        .height = m_WindowExtent.height,
        .depth = 1,
    };

    m_DrawImage.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
    m_DrawImage.Extent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_STORAGE_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    };

    VkImageCreateInfo rImageInfo = vkinit::GetImageCreateInfo(m_DrawImage.Format, drawImageUsages, drawImageExtent);
    VmaAllocationCreateInfo rImageAllocationInfo{
        .flags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    };
    vmaCreateImage(m_Allocator, &rImageInfo, &rImageAllocationInfo, &m_DrawImage.Image, &m_DrawImage.Allocation, nullptr);

    VkImageViewCreateInfo rImageViewInfo = vkinit::GetImageViewCreateInfo(m_DrawImage.Format, m_DrawImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkCreateImageView(m_Device, &rImageViewInfo, nullptr, &m_DrawImage.View));

    m_MainDeletionQueue.pushFunction([this]() {
        vkDestroyImageView(m_Device, m_DrawImage.View, nullptr);
        vmaDestroyImage(m_Allocator, m_DrawImage.Image, m_DrawImage.Allocation);
    });
}

void VulkanEngine::initCommands() {
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::GetCommandPoolCreateInfo(m_GraphicsQueueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (std::uint32_t i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_Frames[i].CommandPool));

        VkCommandBufferAllocateInfo commandAllocateInfo = vkinit::GetCommandBufferAllocateInfo(m_Frames[i].CommandPool);

        VK_CHECK(vkAllocateCommandBuffers(m_Device, &commandAllocateInfo, &m_Frames[i].CommandBuffer));
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
}

void VulkanEngine::initBackgroundPipelines() {
    VkPipelineLayoutCreateInfo computeLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_DrawImageDescriptorLayout,
    };
    VK_CHECK(vkCreatePipelineLayout(m_Device, &computeLayoutInfo, nullptr, &m_GradientPipelineLayout));

    VkShaderModule computeShaderModule{VK_NULL_HANDLE};
    assert(vkutils::LoadShaderModule(m_Device, "Assets/Shaders/Gradient.comp.spv", &computeShaderModule));

    VkComputePipelineCreateInfo computePipelineInfo{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = computeShaderModule,
            .pName = "main",
        },
        .layout = m_GradientPipelineLayout,
    };

    VK_CHECK(vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &m_GradientPipeline));

    vkDestroyShaderModule(m_Device, computeShaderModule, nullptr);

    m_MainDeletionQueue.pushFunction([this]() {
        vkDestroyPipeline(m_Device, m_GradientPipeline, nullptr);
        vkDestroyPipelineLayout(m_Device, m_GradientPipelineLayout, nullptr);
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

FrameData &VulkanEngine::getCurrentFrame() {
    return m_Frames[m_FrameNumber % FRAME_OVERLAP];
}