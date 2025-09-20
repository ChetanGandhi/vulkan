#pragma once

#include "platform.h"
#include "common.h"
#include "logger.h"

typedef struct XrContext
{
    uint32_t swapchainImageCount = 0;
    uint32_t currentFrame = 0;
    VkShaderModule vertexShaderModule;
    VkShaderModule fragmentShaderModule;
    XrLogger *logger = VK_NULL_HANDLE;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugReportCallbackEXT debugReportCallback = VK_NULL_HANDLE;
    XrPhysicalDevice *gpu = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkExtent2D surfaceExtent;
    VkSurfaceFormatKHR surfaceFormat;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    XrSwapchainSupportDetails *swapchainSupportDetails;
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
} XrContext;
