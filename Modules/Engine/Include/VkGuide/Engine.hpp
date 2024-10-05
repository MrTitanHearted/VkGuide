#pragma once

#include <VkGuide/Defines.hpp>

struct FrameData {
    VkCommandPool CommandPool;
    VkCommandBuffer CommandBuffer;

    VkSemaphore SwapchainSemaphore;
    VkSemaphore RenderSemaphore;
    VkFence RenderFence;
};

constexpr std::uint32_t FRAME_OVERLAP{2};

class VulkanEngine {
   public:
    VulkanEngine(const VulkanEngine &) = delete;
    VulkanEngine(VulkanEngine &&) = delete;
    VulkanEngine &operator=(const VulkanEngine &) = delete;
    VulkanEngine &operator=(VulkanEngine &&) = delete;

    void init();
    void cleanup();
    void draw();
    void run();

    static VulkanEngine &GetInstance();

   private:
    VulkanEngine() = default;
    ~VulkanEngine() = default;

    void initVulkan();
    void initSwapchain();
    void initCommands();
    void initSyncStructures();

    void createSwapchain(std::uint32_t width, std::uint32_t height);
    void destroySwapchain();

    FrameData &getCurrentFrame();

   private:
    static VulkanEngine g_VkEngine;

    bool m_IsInitialized{false};
    std::int32_t m_FrameNumber{0};
    bool m_StopRendering{false};

    VkExtent2D m_WindowExtent{1200, 1000};

    VkInstance m_Instance;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
    VkPhysicalDevice m_PhysicalDevice;
    VkDevice m_Device;
    VkSurfaceKHR m_Surface;

    VkSwapchainKHR m_Swapchain;
    VkFormat m_SwapchainImageFormat;

    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;
    VkExtent2D m_SwapchainExtent;

    VkQueue m_GraphicsQueue;
    std::uint32_t m_GraphicsQueueIndex;

    std::array<FrameData, FRAME_OVERLAP> m_Frames;

    struct SDL_Window *m_Window{nullptr};
};