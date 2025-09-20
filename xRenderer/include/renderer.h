#pragma once

#include "platform.h"
#include "common.h"
#include "context.h"
#include "vertex.h"
#include "model.h"

XR_API void xrWaitForIdle(XrContext *context);

XR_API VkResult xrInitInstance(XrContext *context, VkApplicationInfo *applicationInfo);
XR_API VkResult xrDestroyInstance(XrContext *context);

XR_API VkResult xrInitDevice(XrContext *context);
XR_API VkResult xrInitLogicalDevice(XrContext *context);
XR_API VkResult xrDestroyDevice(XrContext *context);

XR_API VkResult xrInitSwapchain(XrContext *context);
XR_API VkResult xrDestroySwapchain(XrContext *context);

XR_API VkResult xrInitSwapchainImageViews(XrContext *context);
XR_API VkResult xrDestroySwapchainImageViews(XrContext *context);

XR_API VkResult xrInitRenderPass(XrContext *context);
XR_API VkResult xrDestroyRenderPass(XrContext *context);

XR_API VkResult xrInitDescriptorSetLayout(XrContext *context);
XR_API VkResult xrDestroyDescriptorSetLayout(XrContext *context);

XR_API VkResult xrInitGraphicsPiplineCache(XrContext *context);
XR_API VkResult xrDestroyGraphicsPiplineCache(XrContext *context);

XR_API VkResult xrInitGraphicsPipline(XrContext *context);
XR_API VkResult xrDestroyGraphicsPipline(XrContext *context);

XR_API VkResult xrInitFrameBuffers(XrContext *context);
XR_API VkResult xrDestroyFrameBuffers(XrContext *context);

XR_API VkResult xrInitCommandPool(XrContext *context);
XR_API VkResult xrDestroyCommandPool(XrContext *context);

XR_API VkResult xrInitDepthStencilImage(XrContext *context);
XR_API VkResult xrDestroyDepthStencilImage(XrContext *context);

XR_API VkResult xrInitMSAAColorImage(XrContext *context);
XR_API VkResult xrDestroyMSAAColorImage(XrContext *context);

XR_API VkResult xrInitTextureImage(XrContext *context, XrModel *model, XrTexture *texture, void *pixels);
XR_API VkResult xrDestroyTextureImage(XrContext *context, XrModel *model);

XR_API VkResult xrInitTextureImageView(XrContext *context, XrModel *model, XrTexture *texture);
XR_API VkResult xrDestroyTextureImageView(XrContext *context, XrModel *model);

XR_API VkResult xrInitTextureSampler(XrContext *context, XrModel *model, XrTexture *texture);
XR_API VkResult xrDestroyTextureSampler(XrContext *context, XrModel *model);

XR_API VkResult xrInitVertexBuffer(XrContext *context, XrModel *model);
XR_API VkResult xrDestroyVertexBuffer(XrContext *context, XrModel *model);

XR_API VkResult xrInitIndexBuffer(XrContext *context, XrModel *model);
XR_API VkResult xrDestroyIndexBuffer(XrContext *context, XrModel *model);

XR_API VkResult xrInitUniformBuffers(XrContext *context, XrModel *model);
XR_API VkResult xrDestroyUniformBuffers(XrContext *context, XrModel *model);

XR_API VkResult xrInitDescriptorPool(XrContext *context, size_t models);
XR_API VkResult xrDestroyDescriptorPool(XrContext *context);

XR_API VkResult xrInitDescriptorSets(XrContext *context, std::vector<XrModel *> &models);
XR_API VkResult xrDestroyDescriptorSets(XrContext *context, std::vector<XrModel *> &models);

XR_API VkResult xrInitCommandBuffers(XrContext *context, std::vector<XrModel *> &models);
XR_API VkResult xrDestroyCommandBuffers(XrContext *context);

XR_API VkResult xrInitSynchronizations(XrContext *context);
XR_API VkResult xrDestroySynchronizations(XrContext *context);

XR_API VkResult xrRecreateSwapChain(XrContext *context, std::vector<XrModel *> &models);
XR_API VkResult xrCleanupSwapChain(XrContext *context, std::vector<XrModel *> &models);

XR_API VkResult xrRender(XrContext *context, std::vector<XrModel *> &models);

XR_API VkResult xrCreateShaderModule(XrContext *context, const char *shaderFilePath, VkShaderModule *shaderModule);
XR_API VkResult xrDestroyShaderModule(XrContext *context, VkShaderModule shaderModule);

XR_API VkResult xrCreateBuffer(
    XrContext *context,
    VkDeviceSize size,
    VkBufferUsageFlags bufferUsage,
    VkMemoryPropertyFlags memoryProperties,
    VkBuffer *buffer,
    VkDeviceMemory *bufferMemory
);

XR_API VkResult xrCreateImage(
    XrContext *context,
    VkExtent2D *imageExtent,
    uint32_t mipLevels,
    VkSampleCountFlagBits samplesCount,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryPropertyFlags,
    VkImage *image,
    VkDeviceMemory *imageMemory
);

XR_API VkResult
xrCreateImageView(XrContext *context, VkImage image, VkFormat format, VkImageView *imageView, VkImageAspectFlags imageAspectFlags, uint32_t mipLevels);

XR_API VkResult xrCopyBuffer(XrContext *context, VkBuffer sourceBuffer, VkBuffer targetBuffer, VkDeviceSize size);

XR_API VkResult xrCopyBufferToImage(XrContext *context, VkBuffer buffer, VkImage image, VkExtent2D *extent);
XR_API void xrListAllPhysicalDevices(XrContext *context, std::vector<XrPhysicalDevice> &gpuList);

XR_API uint32_t xrFindMemoryTypeIndex(
    const VkPhysicalDeviceMemoryProperties *gpuMemoryProperties,
    const VkMemoryRequirements *imageMemoryRequirements,
    const VkMemoryPropertyFlags requiredMemoryProperties
);

XR_API VkResult xrBeginOneTimeCommand(XrContext *context, VkCommandBuffer *commandBuffer);
XR_API VkResult xrEndOneTimeCommand(XrContext *context, VkCommandBuffer *commandBuffer);

// private
void xrSetupLayersAndExtensions(XrContext *context);
void xrUpdateUniformBuffer(XrContext *context, std::vector<XrModel *> &models, uint32_t imageIndex);

VkResult xrGenerateMipmaps(XrContext *context, VkImage image, XrTexture *texture);

VkResult xrTransitionImageLayout(
    XrContext *context,
    VkImage *image,
    VkFormat format,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    uint32_t mipLevels
);

VkResult xrQuerySwapchainSupportDetails(XrContext *context, VkPhysicalDevice gpu, XrSwapchainSupportDetails *details);

VkFormat xrFindSupportedFormat(
    XrContext *context,
    VkPhysicalDevice gpu,
    std::vector<VkFormat> &formatsToCheck,
    VkImageTiling imageTiling,
    VkFormatFeatureFlags formatFeatureFlags
);
VkFormat xrFindDepthFormat(XrContext *context);
void xrChooseSurfaceFormat(std::vector<VkSurfaceFormatKHR> &surfaceFormats, VkSurfaceFormatKHR *format);
VkPresentModeKHR xrChoosePresentMode(XrContext *context, std::vector<VkPresentModeKHR> &presentModes);
void xrChooseSurfaceExtent(VkSurfaceCapabilitiesKHR surfaceCapabilities, VkExtent2D *surfaceExtent);

void xrRankDevice(XrContext *context, XrPhysicalDevice *gpu);
void xrFindSuitableDeviceQueues(XrContext *context, XrPhysicalDevice *gpu);
void xrFindMaxMSAASampleCount(XrContext *context, XrPhysicalDevice *gpu);
bool xrCheckDeviceExtensionSupport(XrContext *context, VkPhysicalDevice gpu);
bool xrHasStencilComponent(XrContext *context, VkFormat format);

// Debug methods

XR_API void xrPrintGpuProperties(XrContext *context, XrPhysicalDevice *details, uint32_t currentGpuIndex, uint32_t totalGpuCount);
XR_API void xrPrintInstanceLayerProperties(XrContext *context, std::vector<VkLayerProperties> &properties);
XR_API void xrPrintDeviceLayerProperties(XrContext *context, std::vector<VkLayerProperties> &properties);
XR_API void xrPrintSurfaceFormatsDetails(XrContext *context, std::vector<VkSurfaceFormatKHR> &surfaceFormats);
XR_API void xrPrintSwapChainImageCount(XrContext *context, uint32_t minImageCount, uint32_t maxImageCount, uint32_t currentImageCount);
