#pragma once

#include <VkGuide/Defines.hpp>
#include <VkGuide/VkDescriptors.hpp>
#include <VkGuide/VkTypes.hpp>

constexpr std::uint32_t FRAME_OVERLAP{2};

class DeletionQueue {
   public:
    DeletionQueue() = default;
    ~DeletionQueue() = default;

    void pushFunction(std::function<void()> &&function) {
        m_Deletors.emplace_back(std::move(function));
    }

    void flush() {
        for (std::deque<std::function<void()>>::reverse_iterator it = m_Deletors.rbegin();
             it != m_Deletors.rend();
             it++) {
            (*it)();
        }

        m_Deletors.clear();
    }

   private:
    std::deque<std::function<void()>> m_Deletors;
};

struct FrameData {
    VkCommandPool CommandPool;
    VkCommandBuffer CommandBuffer;

    VkSemaphore SwapchainSemaphore;
    VkSemaphore RenderSemaphore;
    VkFence RenderFence;

    DeletionQueue DeletionQueue;
};

struct ComputePushConstants {
    glm::vec4 Data1;
    glm::vec4 Data2;
    glm::vec4 Data3;
    glm::vec4 Data4;
};

struct ComputeEffect {
    const char *Name;
    VkPipelineLayout Layout;
    VkPipeline Pipeline;
    ComputePushConstants Data;
};

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
    void initDescriptors();
    void initPipelines();
    void initBackgroundPipelines();
    void initImGui();

    void createSwapchain(std::uint32_t width, std::uint32_t height);
    void destroySwapchain();

    void drawBackground(VkCommandBuffer commandBuffer);
    void drawImGui(VkCommandBuffer commandBuffer, VkImageView targetImageView);

    void immediateSubmit(std::function<void(VkCommandBuffer commandBuffer)> &&function);

    FrameData &getCurrentFrame();

   private:
    static VulkanEngine g_VkEngine;

    bool m_IsInitialized{false};
    std::int32_t m_FrameNumber{0};
    bool m_StopRendering{false};

    struct SDL_Window *m_Window{nullptr};
    VkExtent2D m_WindowExtent{1200, 1000};

    VkInstance m_Instance{VK_NULL_HANDLE};
    VkDebugUtilsMessengerEXT m_DebugMessenger{VK_NULL_HANDLE};
    VkPhysicalDevice m_PhysicalDevice{VK_NULL_HANDLE};
    VkDevice m_Device{VK_NULL_HANDLE};
    VkSurfaceKHR m_Surface{VK_NULL_HANDLE};

    VkSwapchainKHR m_Swapchain{VK_NULL_HANDLE};
    VkFormat m_SwapchainImageFormat{VK_FORMAT_UNDEFINED};

    std::vector<VkImage> m_SwapchainImages{};
    std::vector<VkImageView> m_SwapchainImageViews{};
    VkExtent2D m_SwapchainExtent{};

    VkQueue m_GraphicsQueue{VK_NULL_HANDLE};
    std::uint32_t m_GraphicsQueueIndex{0};

    std::array<FrameData, FRAME_OVERLAP> m_Frames{};

    DeletionQueue m_MainDeletionQueue{};

    VmaAllocator m_Allocator{nullptr};

    AllocatedImage m_DrawImage{};
    VkExtent2D m_DrawExtent{};

    DescriptorAllocator m_GlobalDescriptorAllocator{};

    VkDescriptorSetLayout m_DrawImageDescriptorLayout{VK_NULL_HANDLE};
    VkDescriptorSet m_DrawImageDescriptors{VK_NULL_HANDLE};

    VkPipelineLayout m_GradientPipelineLayout{VK_NULL_HANDLE};
    VkPipeline m_GradientPipeline{VK_NULL_HANDLE};

    VkCommandPool m_ImmCommandPool{VK_NULL_HANDLE};
    VkCommandBuffer m_ImmCommandBuffer{VK_NULL_HANDLE};
    VkFence m_ImmFence{VK_NULL_HANDLE};

    std::vector<ComputeEffect> m_BackgroundEffects{};
    std::uint32_t m_CurrentBackgroundEffect{0U};
};
