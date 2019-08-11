#pragma once

#include "platform.h"
#include "common.h"
#include "vertex.h"
#include "vulkanState.h"

class Renderer
{

public:
    Renderer(VulkanState *vkState);
    ~Renderer();

    void waitForIdle();

    void initInstance();
    void destroyInstance();

    void initDevice();
    void initLogicalDevice();
    void destroyDevice();

    void initSwapchain();
    void destroySwapchain();

    void initSwapchainImageViews();
    void destroySwapchainImageViews();

    void initRenderPass();
    void destroyRenderPass();

    void initDescriptorSetLayout();
    void destroyDescriptorSetLayout();

    void initGraphicsPiplineCache();
    void destroyGraphicsPiplineCache();

    void initGraphicsPipline();
    void destroyGraphicsPipline();

    void initFrameBuffers();
    void destroyFrameBuffers();

    void initCommandPool();
    void destroyCommandPool();

    void initDepthStencilImage();
    void destoryDepthStencilImage();

    void initMSAAColorImage();
    void destoryMSAAColorImage();

    void initTextureImage();
    void destroyTextureImage();

    void initTextureImageView();
    void destroyTextureImageView();

    void initTextureSampler();
    void destoryTextureSampler();

    void loadModel();

    void initVertexBuffer();
    void destroyVertexBuffer();

    void initIndexBuffer();
    void destroyIndexBuffer();

    void initUniformBuffers();
    void destroyUniformBuffers();

    void initDescriptorPool();
    void destroyDescriptorPool();

    void initDescriptorSets();
    void destroyDescriptorSets();

    void initCommandBuffers();
    void destroyCommandBuffers();

    void initSynchronizations();
    void destroySynchronizations();

    void recreateSwapChain();
    void cleanupSwapChain();

    void render();

private:
    VulkanState *vkState = nullptr;

    const std::string chaletModelResourcePath = "resources/models/chalet/chalet.obj";
    const std::string chaletTextureResourcePath = "resources/textures/chalet/chalet.jpg";

    VkDebugReportCallbackEXT debugReport = VK_NULL_HANDLE;
    VkDebugReportCallbackCreateInfoEXT debugReportCallbackInfo = {};

    void setupDebugLayer();
    void setupLayersAndExtensions();

    void enableDebug();
    void disableDebug();

    void beginOneTimeCommand(VkCommandBuffer &commandBuffer);
    void endOneTimeCommand(VkCommandBuffer &commandBuffer);
    void updateUniformBuffer(uint32_t imageIndex);

    void generateMipmaps(VkImage &image, int32_t textureWidth, int32_t textureHeight, uint32_t mipLevels);

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, uint32_t mipLevels);
    void listAllPhysicalDevices(std::vector<GpuDetails> *gpuDetailsList);
    void querySwapchainSupportDetails(VkPhysicalDevice gpu, SwapchainSupportDetails *details);

    VkFormat findSupportedFormat(VkPhysicalDevice gpu, const std::vector<VkFormat> &formatsToCheck, VkImageTiling imageTiling, VkFormatFeatureFlags formatFeatureFlags);
    VkFormat findDepthFormat();
    VkSampleCountFlagBits findMaxMSAASampleCount(VkPhysicalDeviceProperties properties);
    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &surfaceFormats);
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> &presentModes);
    void chooseSurfaceExtent(VkSurfaceCapabilitiesKHR surfaceCapabilities, VkExtent2D *initialSurfaceExtent);

    bool isDeviceSuitable(VkPhysicalDevice gpu);
    bool findSuitableDeviceQueues(VkPhysicalDevice gpu, QueueFamilyIndices *queueFamilyIndices);
    bool checkDeviceExtensionSupport(VkPhysicalDevice gpu);
    bool hasStencilComponent(VkFormat format);

    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits samplesCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, VkImage &image, VkDeviceMemory &imageMemory);
    void createImageView(VkImage image, VkFormat format, VkImageView &imageView, VkImageAspectFlags imageAspectFlags, uint32_t mipLevels);
    void copyBuffer(VkBuffer sourceBuffer, VkBuffer targetBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    // Debug methods

    void printGpuProperties(VkPhysicalDeviceProperties *properties, uint32_t currentGpuIndex, uint32_t totalGpuCount);
    void printInstanceLayerProperties(std::vector<VkLayerProperties> properties);
    void printDeviceLayerProperties(std::vector<VkLayerProperties> properties);
    void printSurfaceFormatsDetails(std::vector<VkSurfaceFormatKHR> surfaceFormats);
    void printSwapChainImageCount(uint32_t minImageCount, uint32_t maxImageCount, uint32_t currentImageCount);
};
