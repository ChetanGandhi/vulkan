#pragma once

#include "platform.h"
#include "common.h"
#include "vertex.h"
#include "model.h"
#include "context.h"

namespace xr
{
    class Renderer
    {
      public:
        XR_API Renderer(Context *context);

        XR_API void waitForIdle(Context *context);

        XR_API void initInstance(Context *context);
        XR_API void destroyInstance(Context *context);

        XR_API void initDevice(Context *context);
        XR_API void initLogicalDevice(Context *context);
        XR_API void destroyDevice(Context *context);

        XR_API void initSwapchain(Context *context);
        XR_API void destroySwapchain(Context *context);

        XR_API void initSwapchainImageViews(Context *context);
        XR_API void destroySwapchainImageViews(Context *context);

        XR_API void initRenderPass(Context *context);
        XR_API void destroyRenderPass(Context *context);

        XR_API void initDescriptorSetLayout(Context *context);
        XR_API void destroyDescriptorSetLayout(Context *context);

        XR_API void initGraphicsPiplineCache(Context *context);
        XR_API void destroyGraphicsPiplineCache(Context *context);

        XR_API void initGraphicsPipline(Context *context);
        XR_API void destroyGraphicsPipline(Context *context);

        XR_API void initFrameBuffers(Context *context);
        XR_API void destroyFrameBuffers(Context *context);

        XR_API void initCommandPool(Context *context);
        XR_API void destroyCommandPool(Context *context);

        XR_API void initDepthStencilImage(Context *context);
        XR_API void destroyDepthStencilImage(Context *context);

        XR_API void initMSAAColorImage(Context *context);
        XR_API void destroyMSAAColorImage(Context *context);

        XR_API void initTextureImage(Context *context, Model *model, Texture *texture, void *pixels);
        XR_API void destroyTextureImage(Context *context, Model *model);

        XR_API void initTextureImageView(Context *context, Model *model, Texture *texture);
        XR_API void destroyTextureImageView(Context *context, Model *model);

        XR_API void initTextureSampler(Context *context, Model *model, Texture *texture);
        XR_API void destroyTextureSampler(Context *context, Model *model);

        XR_API void initVertexBuffer(Context *context, Model *model);
        XR_API void destroyVertexBuffer(Context *context, Model *model);

        XR_API void initIndexBuffer(Context *context, Model *model);
        XR_API void destroyIndexBuffer(Context *context, Model *model);

        XR_API void initUniformBuffers(Context *context, Model *model);
        XR_API void destroyUniformBuffers(Context *context, Model *model);

        XR_API void initDescriptorPool(Context *context, size_t models);
        XR_API void destroyDescriptorPool(Context *context);

        XR_API void initDescriptorSets(Context *context, std::vector<Model *> models);
        XR_API void destroyDescriptorSets(Context *context, std::vector<Model *> models);

        XR_API void initCommandBuffers(Context *context, std::vector<Model *> models);
        XR_API void destroyCommandBuffers(Context *context);

        XR_API void initSynchronizations(Context *context);
        XR_API void destroySynchronizations(Context *context);

        XR_API void recreateSwapChain(Context *context, std::vector<Model *> models);
        XR_API void cleanupSwapChain(Context *context, std::vector<Model *> models);

        XR_API void render(Context *context, std::vector<Model *> models);

        XR_API VkShaderModule createShaderModule(Context *context, const std::vector<char> &code);

        XR_API void createBuffer(
            Context *context,
            VkDeviceSize size,
            VkBufferUsageFlags bufferUsage,
            VkMemoryPropertyFlags memoryProperties,
            VkBuffer *buffer,
            VkDeviceMemory *bufferMemory
        );

        XR_API void createImage(
            Context *context,
            uint32_t width,
            uint32_t height,
            uint32_t mipLevels,
            VkSampleCountFlagBits samplesCount,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags memoryPropertyFlags,
            VkImage &image,
            VkDeviceMemory &imageMemory
        );

        XR_API void createImageView(
            Context *context,
            VkImage image,
            VkFormat format,
            VkImageView &imageView,
            VkImageAspectFlags imageAspectFlags,
            uint32_t mipLevels
        );

        XR_API void copyBuffer(Context *context, VkBuffer sourceBuffer, VkBuffer targetBuffer, VkDeviceSize size);

        XR_API void copyBufferToImage(Context *context, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        XR_API void listAllPhysicalDevices(Context *context, std::vector<GpuDetails> *gpuDetailsList);

      private:
        void setupLayersAndExtensions(Context *context);
        void beginOneTimeCommand(Context *context, VkCommandBuffer &commandBuffer);
        void endOneTimeCommand(Context *context, VkCommandBuffer &commandBuffer);
        void updateUniformBuffer(Context *context, std::vector<Model *> models, uint32_t imageIndex);

        void generateMipmaps(Context *context, VkImage &image, int32_t textureWidth, int32_t textureHeight, uint32_t mipLevels);

        void transitionImageLayout(
            Context *context,
            VkImage image,
            VkFormat format,
            VkImageLayout oldImageLayout,
            VkImageLayout newImageLayout,
            uint32_t mipLevels
        );
        void querySwapchainSupportDetails(Context *context, VkPhysicalDevice gpu, SwapchainSupportDetails *details);

        VkFormat findSupportedFormat(
            VkPhysicalDevice gpu,
            const std::vector<VkFormat> &formatsToCheck,
            VkImageTiling imageTiling,
            VkFormatFeatureFlags formatFeatureFlags
        );
        VkFormat findDepthFormat(Context *context);
        VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &surfaceFormats);
        VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> &presentModes);
        void chooseSurfaceExtent(VkSurfaceCapabilitiesKHR surfaceCapabilities, VkExtent2D *initialSurfaceExtent);

        void rankDevice(Context *context, GpuDetails *gpuDetails);
        void findSuitableDeviceQueues(Context *context, GpuDetails *gpuDetails);
        void findMaxMSAASampleCount(GpuDetails *gpuDetails);
        bool checkDeviceExtensionSupport(Context *context, VkPhysicalDevice gpu);
        bool hasStencilComponent(VkFormat format);

        // Debug methods

        void printGpuProperties(GpuDetails *details, uint32_t currentGpuIndex, uint32_t totalGpuCount);
        void printInstanceLayerProperties(std::vector<VkLayerProperties> properties);
        void printDeviceLayerProperties(std::vector<VkLayerProperties> properties);
        void printSurfaceFormatsDetails(std::vector<VkSurfaceFormatKHR> surfaceFormats);
        void printSwapChainImageCount(uint32_t minImageCount, uint32_t maxImageCount, uint32_t currentImageCount);
    };
} // namespace xr
