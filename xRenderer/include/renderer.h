#pragma once

#include "platform.h"
#include "common.h"
#include "vertex.h"
#include "vulkanState.h"

namespace xr {
    class Renderer
    {

    public:
        XR_API Renderer(VulkanState *vkState);
        XR_API ~Renderer();

        XR_API void waitForIdle();

        XR_API void initInstance();
        XR_API void destroyInstance();

        XR_API void initDevice();
        XR_API void initLogicalDevice();
        XR_API void destroyDevice();

        XR_API void initSwapchain();
        XR_API void destroySwapchain();

        XR_API void initSwapchainImageViews();
        XR_API void destroySwapchainImageViews();

        XR_API void initRenderPass();
        XR_API void destroyRenderPass();

        XR_API void initDescriptorSetLayout();
        XR_API void destroyDescriptorSetLayout();

        XR_API void initGraphicsPiplineCache();
        XR_API void destroyGraphicsPiplineCache();

        XR_API void initGraphicsPipline();
        XR_API void destroyGraphicsPipline();

        XR_API void initFrameBuffers();
        XR_API void destroyFrameBuffers();

        XR_API void initCommandPool();
        XR_API void destroyCommandPool();

        XR_API void initDepthStencilImage();
        XR_API void destoryDepthStencilImage();

        XR_API void initMSAAColorImage();
        XR_API void destoryMSAAColorImage();

        XR_API void initTextureImage(const char* textureFilePath);
        XR_API void destroyTextureImage();

        XR_API void initTextureImageView();
        XR_API void destroyTextureImageView();

        XR_API void initTextureSampler();
        XR_API void destoryTextureSampler();

        XR_API void loadModel(const char* modelFilePath);

        XR_API void initVertexBuffer();
        XR_API void destroyVertexBuffer();

        XR_API void initIndexBuffer();
        XR_API void destroyIndexBuffer();

        XR_API void initUniformBuffers();
        XR_API void destroyUniformBuffers();

        XR_API void initDescriptorPool();
        XR_API void destroyDescriptorPool();

        XR_API void initDescriptorSets();
        XR_API void destroyDescriptorSets();

        XR_API void initCommandBuffers();
        XR_API void destroyCommandBuffers();

        XR_API void initSynchronizations();
        XR_API void destroySynchronizations();

        XR_API void recreateSwapChain();
        XR_API void cleanupSwapChain();

        XR_API void render();

        XR_API VkShaderModule createShaderModule(const std::vector<char>& code);
        XR_API void createBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, VkBuffer *buffer, VkDeviceMemory *bufferMemory);
        XR_API void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits samplesCount, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, VkImage &image, VkDeviceMemory &imageMemory);
        XR_API void createImageView(VkImage image, VkFormat format, VkImageView &imageView, VkImageAspectFlags imageAspectFlags, uint32_t mipLevels);
        XR_API void copyBuffer(VkBuffer sourceBuffer, VkBuffer targetBuffer, VkDeviceSize size);
        XR_API void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    private:
        VulkanState *vkState = nullptr;

        void setupLayersAndExtensions();
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

        // Debug methods

        void printGpuProperties(VkPhysicalDeviceProperties *properties, uint32_t currentGpuIndex, uint32_t totalGpuCount);
        void printInstanceLayerProperties(std::vector<VkLayerProperties> properties);
        void printDeviceLayerProperties(std::vector<VkLayerProperties> properties);
        void printSurfaceFormatsDetails(std::vector<VkSurfaceFormatKHR> surfaceFormats);
        void printSwapChainImageCount(uint32_t minImageCount, uint32_t maxImageCount, uint32_t currentImageCount);
    };
}
