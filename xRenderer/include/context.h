#pragma once

#include "platform.h"
#include "common.h"
#include "logger.h"

typedef struct XrContext
{
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    const char *vertexShaderFilePath = VK_NULL_HANDLE;
    const char *fragmentShaderFile = VK_NULL_HANDLE;
    XrLogger *logger = VK_NULL_HANDLE;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT pDebugMessenger = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;
    VkImage msaaColorImage = VK_NULL_HANDLE;
    VkDeviceMemory msaaColorImageMemory = VK_NULL_HANDLE;
    VkImageView msaaColorImageView = VK_NULL_HANDLE;

    std::vector<const char *> instanceLayers;
    std::vector<const char *> instanceExtensions;
    std::vector<const char *> deviceExtensions;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> commandBuffers;

    uint32_t swapchainImageCount = 2;
    size_t currentFrame = 0;

    VkSurfaceFormatKHR surfaceFormat = {};
    XrSwapchainSupportDetails swapchainSupportDetails = {};
    XrGpuDetails gpuDetails = {};
    VkExtent2D surfaceSize = {};
} XrContext;
