#include <VkGuide/Engine.hpp>
#include <VkGuide/VkInitializers.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <VkBootstrap.h>

#include <chrono>
#include <thread>

#if defined(VKGUIDE_BUILD_TYPE_RELEASE)
constexpr bool g_UseValidationLayers{false};
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

    m_IsInitialized = true;
}

void VulkanEngine::cleanup() {
    if (!m_IsInitialized) return;

    vkDeviceWaitIdle(m_Device);
    for (FrameData &frame : m_Frames) {
        vkDestroyCommandPool(m_Device, frame.CommandPool, nullptr);
    }

    destroySwapchain();

    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyDevice(m_Device, nullptr);

    vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
    vkDestroyInstance(m_Instance, nullptr);

    SDL_DestroyWindow(m_Window);
}

void VulkanEngine::draw() {
}

void VulkanEngine::run() {
    SDL_Event e{0};
    bool quit{false};

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
                m_StopRendering = true;
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
}

void VulkanEngine::initSwapchain() {
    createSwapchain(m_WindowExtent.width, m_WindowExtent.height);
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
            .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
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