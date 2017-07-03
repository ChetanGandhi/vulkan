#pragma once

#include "platform.h"
#include "common.h"
#include <vector>
#include <string>

class Renderer
{

public:
    Renderer(SurfaceSize surfaceSize);
    ~Renderer();

    void setSurface(VkSurfaceKHR surface);
    void waitForIdle();

    void initDevice();
    void initLogicalDevice();
    void destroyDevice();

    void initSwapchain();
    void destroySwapchain();

    void initSwapchainImages();
    void destroySwapchainImages();

    void initRenderPass();
    void destroyRenderPass();

    void initGraphicsPipline();
    void destroyGraphicsPipline();

    // void initDepthStencilImage();
    // void destoryDepthStencilImage();

    void initFrameBuffers();
    void destroyFrameBuffers();

    void initCommandPool();
    void destroyCommandPool();

    void initCommandBuffers();
    void destroyCommandBuffers();

    void initSynchronizations();
    void destroySynchronizations();

    void render();

    const VkInstance getVulkanInstance() const;
    const VkPhysicalDevice getVulkanPhysicalDevice() const;
    const VkDevice getVulkanDevice() const;
    const VkQueue getVulkanGraphicsQueue() const;
    const VkPhysicalDeviceProperties &getVulkanPhysicalDeviceProperties() const;
    const VkPhysicalDeviceMemoryProperties &getVulkanPhysicalDeviceMemoryProperties() const;
    const QueueFamilyIndices getQueueFamilyIndices() const;
    const VkRenderPass getVulkanRenderPass() const;

private:
   SurfaceSize surfaceSize;

   VkInstance instance = VK_NULL_HANDLE;
   VkDevice device = VK_NULL_HANDLE;
   VkSurfaceKHR surface = VK_NULL_HANDLE;
   VkQueue graphicsQueue = VK_NULL_HANDLE;
   VkQueue presentQueue = VK_NULL_HANDLE;

   struct GpuDetails {
        VkPhysicalDevice gpu = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties properties = {};
        VkPhysicalDeviceMemoryProperties memoryProperties = {};
    } gpuDetails;

    VkDebugReportCallbackEXT debugReport = VK_NULL_HANDLE;
    VkDebugReportCallbackCreateInfoEXT debugReportCallbackInfo = {};

    QueueFamilyIndices queueFamilyIndices;

    std::vector<const char*> instanceLayerList;
    std::vector<const char*> deviceLayerList;
    std::vector<const char*> instanceExtensionList;
    std::vector<const char*> deviceExtensionList;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> commandBuffers;

    uint32_t swapchainImageCount = 2;

    // bool stencilAvailable = false;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;

    // struct DepthStencil {
    //     VkImage image = VK_NULL_HANDLE;
    //     VkImageView imageView = VK_NULL_HANDLE;
    //     VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    // } depthStencil;

    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
        std::vector<VkSurfaceFormatKHR> surfaceFormats;
        std::vector<VkPresentModeKHR> presentModes;
    } swapchainSupportDetails;

    VkSurfaceFormatKHR surfaceFormat = {};
    // VkFormat depthStencilFormat = VK_FORMAT_UNDEFINED;

    void setupDebugLayer();
    void setupLayersAndExtensions();

    void enableDebug();
    void disableDebug();

    void initInstance();
    void destroyInstance();

    void listAllPhysicalDevices(std::vector<GpuDetails> *gpuDetailsList);
    void querySwapchainSupportDetails(VkPhysicalDevice gpu, SwapchainSupportDetails *details);

    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &surfaceFormats);
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> presentModes);
    void chooseSurfaceExtent(VkSurfaceCapabilitiesKHR surfaceCapabilities, VkExtent2D *initialSurfaceExtent);

    bool isDeviceSuitable(VkPhysicalDevice gpu);
    bool findSuitableDeviceQueues(VkPhysicalDevice gpu, QueueFamilyIndices *queueFamilyIndices);
    bool checkDeviceExtensionSupport(VkPhysicalDevice gpu);

    VkShaderModule createShaderModule(const std::vector<char>& code);

// Debug methods

    void printGpuProperties(VkPhysicalDeviceProperties *properties, uint32_t currentGpuIndex, uint32_t totalGpuCount);
    void printInstanceLayerProperties(std::vector<VkLayerProperties> properties);
    void printDeviceLayerProperties(std::vector<VkLayerProperties> properties);
    void printSurfaceFormatsDetails(std::vector<VkSurfaceFormatKHR> surfaceFormats);
    void printSwapChainImageCount(uint32_t minImageCount, uint32_t maxImageCount, uint32_t currentImageCount);
};
