#pragma once

#include "platform.h"
#include "common.h"
#include "logger.h"

typedef struct XrContext
{
    VkShaderModule vertexShaderModule;
    VkShaderModule fragmentShaderModule;
    XrLogger *logger = VK_NULL_HANDLE;
    VkInstance instance = VK_NULL_HANDLE;
    VkBool32 enableValidations = VK_FALSE;
    VkDebugReportCallbackEXT debugReportCallback = VK_NULL_HANDLE;
    XrPhysicalDevice *gpu = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkExtent2D surfaceExtent;
    VkSurfaceFormatKHR surfaceFormat;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    XrSwapchainSupport *swapchainSupport = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkFormat depthStencilFormat = VK_FORMAT_UNDEFINED;
    XrImage *depthImage = VK_NULL_HANDLE;
    XrImage *msaaColorImage = VK_NULL_HANDLE;

    char **instanceLayers = VK_NULL_HANDLE;
    uint32_t instanceLayersCount;

    char **instanceExtensions = VK_NULL_HANDLE;
    uint32_t instanceExtensionsCount = 0;
    uint32_t enabledInstanceExtensionCount = 0;

    char **deviceExtensions = VK_NULL_HANDLE;
    uint32_t deviceExtensionsCount = 0;

    VkClearValue *clearValue = VK_NULL_HANDLE;
    uint32_t clearValueCount = 0;

    VkImage *swapchainImages = VK_NULL_HANDLE;
    VkImageView *swapchainImageViews = VK_NULL_HANDLE;
    VkSemaphore *imageAvailableSemaphores = VK_NULL_HANDLE;
    VkSemaphore *renderFinishedSemaphores = VK_NULL_HANDLE;
    VkFence *inFlightFences = VK_NULL_HANDLE;
    VkFramebuffer *framebuffers = VK_NULL_HANDLE;
    VkCommandBuffer *commandBuffers = VK_NULL_HANDLE;
    uint32_t swapchainImageCount = 0;
    uint32_t currentFrame = 0;

} XrContext;
