#pragma once

#include "platform.h"
#include "common.h"
#include "context.h"
#include "vertex.h"
#include "model.h"

XR_API void xrWaitForIdle(XrContext *context);

XR_API VkResult xrSetupLayersAndExtensions(XrContext *context);
XR_API VkResult xrInitInstance(XrContext *context, VkApplicationInfo *applicationInfo);
XR_API void xrDestroyInstance(XrContext *context);

XR_API VkResult xrInitLogicalDevice(XrContext *context);
XR_API void xrDestroyDevice(XrContext *context);

XR_API VkResult xrInitSwapchain(XrContext *context);
XR_API void xrDestroySwapchain(XrContext *context);

XR_API VkResult xrInitSwapchainImageViews(XrContext *context);
XR_API void xrDestroySwapchainImageViews(XrContext *context);

XR_API VkResult xrInitRenderPass(XrContext *context);
XR_API void xrDestroyRenderPass(XrContext *context);

XR_API VkResult xrInitDescriptorSetLayout(XrContext *context);
XR_API void xrDestroyDescriptorSetLayout(XrContext *context);

XR_API VkResult xrInitGraphicsPiplineCache(XrContext *context);
XR_API void xrDestroyGraphicsPiplineCache(XrContext *context);

XR_API VkResult xrInitGraphicsPipline(XrContext *context);
XR_API void xrDestroyGraphicsPipline(XrContext *context);

XR_API VkResult xrInitFrameBuffers(XrContext *context);
XR_API void xrDestroyFrameBuffers(XrContext *context);

XR_API VkResult xrInitCommandPool(XrContext *context);
XR_API void xrDestroyCommandPool(XrContext *context);

XR_API VkResult xrInitDepthStencilImage(XrContext *context);
XR_API void xrDestroyDepthStencilImage(XrContext *context);

XR_API VkResult xrInitMSAAColorImage(XrContext *context);
XR_API void xrDestroyMSAAColorImage(XrContext *context);

XR_API VkResult xrInitTextureImage(XrContext *context, XrTexture *texture, void *pixels);
XR_API void xrDestroyTextureImage(XrContext *context, XrTexture *texture);

XR_API VkResult xrInitTextureImageView(XrContext *context, XrTexture *texture);
XR_API void xrDestroyTextureImageView(XrContext *context, XrTexture *texture);

XR_API VkResult xrInitTextureSampler(XrContext *context, XrTexture *texture);
XR_API void xrDestroyTextureSampler(XrContext *context, XrTexture *texture);

XR_API VkResult xrInitVertexBuffer(XrContext *context, XrModel *model);
XR_API void xrDestroyVertexBuffer(XrContext *context, XrModel *model);

XR_API VkResult xrInitIndexBuffer(XrContext *context, XrModel *model);
XR_API void xrDestroyIndexBuffer(XrContext *context, XrModel *model);

XR_API VkResult xrInitUniformBuffers(XrContext *context, XrModel *model);
XR_API void xrDestroyUniformBuffers(XrContext *context, XrModel *model);

XR_API VkResult xrInitDescriptorPool(XrContext *context, uint32_t count);
XR_API void xrDestroyDescriptorPool(XrContext *context);

XR_API VkResult xrInitDescriptorSets(XrContext *context, XrModel **models, uint32_t moduleCount);
XR_API void xrDestroyDescriptorSets(XrContext *context, XrModel **models, uint32_t moduleCount);

XR_API VkResult xrInitCommandBuffers(XrContext *context, XrModel **models, uint32_t moduleCount);
XR_API void xrDestroyCommandBuffers(XrContext *context);

XR_API VkResult xrInitSynchronizations(XrContext *context);
XR_API void xrDestroySynchronizations(XrContext *context);

XR_API void xrCleanupSwapChain(XrContext *context, XrModel **models, uint32_t moduleCount);
XR_API VkResult xrRecreateSwapChain(XrContext *context, XrModel **models, uint32_t moduleCount);

XR_API VkResult xrRender(XrContext *context, XrModel **models, uint32_t moduleCount);

XR_API VkResult xrCreateShaderModule(XrContext *context, const char *shaderFilePath, VkShaderModule *shaderModule);
XR_API void xrDestroyShaderModule(XrContext *context, VkShaderModule *shaderModule);

XR_API VkResult xrCreateBuffer(XrContext *context, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, XrBuffer *buffer);
XR_API void xrDestroyBuffer(XrContext *context, XrBuffer *buffer);

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
xrCreateImageView(XrContext *context, VkImage image, VkFormat format, VkImageAspectFlags imageAspectFlags, uint32_t mipLevels, VkImageView *imageView);

XR_API VkResult xrCopyBuffer(XrContext *context, VkBuffer sourceBuffer, VkBuffer targetBuffer, VkDeviceSize size);
XR_API VkResult xrCopyBufferToImage(XrContext *context, VkBuffer buffer, VkImage image, VkExtent2D *extent);

XR_API uint32_t xrFindMemoryTypeIndex(
    VkPhysicalDeviceMemoryProperties *gpuMemoryProperties,
    VkMemoryRequirements *imageMemoryRequirements,
    VkMemoryPropertyFlags requiredMemoryProperties
);

XR_API VkResult xrBeginOneTimeCommand(XrContext *context, VkCommandBuffer *commandBuffer);
XR_API VkResult xrEndOneTimeCommand(XrContext *context, VkCommandBuffer *commandBuffer);

XR_API VkResult xrGenerateMipmaps(XrContext *context, VkImage *image, XrTexture *texture);

XR_API VkResult
xrTransitionImageLayout(XrContext *context, VkImage *image, VkFormat format, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, uint32_t mipLevels);

XR_API VkResult xrQuerySwapchainSupportDetails(XrContext *context, VkPhysicalDevice gpu, XrSwapchainSupport *details);

XR_API void xrFindSupportedFormat(
    VkPhysicalDevice gpu,
    VkFormat *formats,
    uint32_t formatsCount,
    VkImageTiling imageTiling,
    VkFormatFeatureFlags formatFeatureFlags,
    VkFormat *supportedFormat
);
XR_API void xrFindDepthFormat(XrContext *context, VkFormat *format);
XR_API void xrChooseSurfaceFormat(VkSurfaceFormatKHR *surfaceFormats, uint32_t surfaceFormatsCount, VkSurfaceFormatKHR *format);
XR_API void xrChoosePresentMode(XrContext *context, VkPresentModeKHR *presentModes, uint32_t presentModesCount, VkPresentModeKHR *presentMode);
XR_API void xrChooseSurfaceExtent(VkSurfaceCapabilitiesKHR surfaceCapabilities, VkExtent2D *surfaceExtent);
XR_API VkBool32 xrHasStencilComponent(VkFormat format);
XR_API VkBool32 xrIsValidationLayerSupport(XrContext *context, const char *validationLayerName);

// Debug methods

XR_API void xrPrintGpuProperties(XrContext *context, XrPhysicalDevice *gpu, uint32_t currentGpuIndex, uint32_t totalGpuCount);
XR_API void xrPrintInstanceLayerProperties(XrContext *context, VkLayerProperties *properties, uint32_t propertiesCount);
XR_API void xrPrintDeviceLayerProperties(XrContext *context, VkLayerProperties *properties, uint32_t propertiesCount);
XR_API void xrPrintSurfaceFormatsDetails(XrContext *context, VkSurfaceFormatKHR *surfaceFormats, uint32_t surfaceFormatsCount);
XR_API void xrPrintSwapChainImageCount(XrContext *context, uint32_t minImageCount, uint32_t maxImageCount, uint32_t currentImageCount);

// private
VkResult xrFillInstanceExtensionNames(XrContext *context);
void xrUpdateUniformBuffer(XrContext *context, XrModel **models, uint32_t moduleCount, uint32_t imageIndex);
