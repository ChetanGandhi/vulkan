#include "platform.h"
#include "renderer.h"
#include "utils.h"
#include "logger.h"
#include "debugger.h"
#include "instance.h"

XR_API VkResult xrInitInstance(XrContext *context, VkApplicationInfo *applicationInfo)
{
    return xrCreateVulkanInstance(
        context, applicationInfo, context->instanceLayers, context->instanceLayersCount, context->instanceExtensions, context->enabledInstanceExtensionCount
    );
}

XR_API void xrDestroyInstance(XrContext *context)
{
    xrDestroyVulkanInstance(context);
    XR_FREE(context->instanceExtensions);
    XR_FREE(context->instanceLayers);
    XR_FREE(context->deviceExtensions);
}

XR_API VkResult xrSetupLayersAndExtensions(XrContext *context)
{
    VkResult vkResult = VK_SUCCESS;

    vkResult = xrFillInstanceExtensionNames(context);

    if (XR_IS_ERROR(vkResult))
    {
        XR_LOG_ERROR(context->logger, "Failed to query instance extensions");
        return vkResult;
    }

    context->deviceExtensionsCount = 1;
    context->deviceExtensions = (char **)malloc(sizeof(char *) * context->deviceExtensionsCount);
    memset((void *)context->deviceExtensions, 0, context->deviceExtensionsCount);
    context->deviceExtensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    VkBool32 validationLayerSupport = xrIsValidationLayerSupport(context, "VK_LAYER_KHRONOS_validation");

    if (validationLayerSupport == VK_FALSE)
    {
        context->instanceLayersCount = 1;
        context->instanceLayers = (char **)malloc(sizeof(char *) * context->instanceLayersCount);
        memset((void *)context->instanceLayers, 0, sizeof(char *) * context->instanceLayersCount);
        context->instanceLayers[0] = "VK_LAYER_KHRONOS_validation";
    }

    return vkResult;
}

VkResult xrFillInstanceExtensionNames(XrContext *context)
{
    VkResult vkResult = VK_SUCCESS;

    uint32_t extensionPropertiesCount = 0;
    VkExtensionProperties *extensionProperties = VK_NULL_HANDLE;

    vkResult = vkEnumerateInstanceExtensionProperties(NULL, &extensionPropertiesCount, NULL);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    extensionProperties = (VkExtensionProperties *)malloc(sizeof(VkExtensionProperties) * extensionPropertiesCount);
    memset((void *)extensionProperties, 0, sizeof(VkExtensionProperties) * extensionPropertiesCount);

    vkResult = vkEnumerateInstanceExtensionProperties(NULL, &extensionPropertiesCount, extensionProperties);

    if (XR_IS_ERROR(vkResult))
    {
        XR_FREE(extensionProperties);

        XR_LOG_ERROR(context->logger, "Failed to query instance extension properties");
        return vkResult;
    }

    for (uint32_t index = 0; index < extensionPropertiesCount; ++index)
    {
        XR_LOG_INFO(context->logger, "Instance extension name [%d]: %s", index, extensionProperties[index].extensionName);
    }

    VkBool32 vulkanSurfaceExtensionFound = VK_FALSE;
    VkBool32 platformSurfaceExtensionFound = VK_FALSE;
    VkBool32 debugReportExtensionFound = VK_FALSE;

    context->enabledInstanceExtensionCount = 0;
    context->instanceExtensionsCount = 3;
    context->instanceExtensions = (char **)malloc(sizeof(char *) * context->instanceExtensionsCount);
    memset((void *)context->instanceExtensions, 0, sizeof(char *) * context->instanceExtensionsCount);

    for (uint32_t index = 0; index < extensionPropertiesCount; ++index)
    {
        VkExtensionProperties *nextExtensionProperties = &extensionProperties[index];

        if (strcmp(nextExtensionProperties->extensionName, VK_KHR_SURFACE_EXTENSION_NAME) == 0)
        {
            vulkanSurfaceExtensionFound = VK_TRUE;
            context->instanceExtensions[context->enabledInstanceExtensionCount++] = VK_KHR_SURFACE_EXTENSION_NAME;
        }

        if (strcmp(nextExtensionProperties->extensionName, PLATFORM_SURFACE_EXTENSION_NAME) == 0)
        {
            platformSurfaceExtensionFound = VK_TRUE;
            context->instanceExtensions[context->enabledInstanceExtensionCount++] = PLATFORM_SURFACE_EXTENSION_NAME;
        }

        if (strcmp(nextExtensionProperties->extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0)
        {
            debugReportExtensionFound = VK_TRUE;

            if (context->enableValidations == VK_TRUE)
            {
                context->instanceExtensions[context->enabledInstanceExtensionCount++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
            }
        }
    }

    XR_FREE(extensionProperties);

    if (vulkanSurfaceExtensionFound == VK_FALSE)
    {
        XR_LOG_ERROR(context->logger, "Vulkan surface extension not found");
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return vkResult;
    }

    if (platformSurfaceExtensionFound == VK_FALSE)
    {
        XR_LOG_ERROR(context->logger, "Platform specific surface extension not found");
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return vkResult;
    }

    if (debugReportExtensionFound == VK_FALSE)
    {
        if (context->enableValidations == VK_TRUE)
        {
            XR_LOG_ERROR(context->logger, "Validation is enabled but debug report extension is not supported");

            vkResult = VK_ERROR_INITIALIZATION_FAILED;
            return vkResult;
        }
        else
        {
            XR_LOG_INFO(context->logger, "Validation is disable and debug report extension is not supported");
        }
    }
    else
    {
        if (context->enableValidations == VK_TRUE)
        {
            XR_LOG_INFO(context->logger, "Validation is enabled and debug report extension is supported");
        }
        else
        {
            XR_LOG_INFO(context->logger, "Validation is disabled but debug report extension is supported");
        }
    }

    XR_LOG_INFO(context->logger, "Enabled extension count: %d", context->enabledInstanceExtensionCount);

    for (uint32_t index = 0; index < context->enabledInstanceExtensionCount; ++index)
    {
        XR_LOG_INFO(context->logger, "Enabled extension name [%d]: %s", index, context->instanceExtensions[index]);
    }

    return vkResult;
}

XR_API VkBool32 xrIsValidationLayerSupport(XrContext *context, const char *validationLayerName)
{
    VkBool32 validationLayerFound = VK_FALSE;
    VkResult vkResult = VK_SUCCESS;
    uint32_t validationLayerCount = 0;
    vkResult = vkEnumerateInstanceLayerProperties(&validationLayerCount, VK_NULL_HANDLE);

    if (XR_IS_ERROR(vkResult))
    {
        validationLayerFound = VK_FALSE;
        return validationLayerFound;
    }

    XR_LOG_INFO(context->logger, "Validation layer count: %d", validationLayerCount);

    VkLayerProperties *vkLayerProperties = (VkLayerProperties *)malloc(sizeof(VkLayerProperties) * validationLayerCount);
    memset((void *)vkLayerProperties, 0, sizeof(VkLayerProperties) * validationLayerCount);
    vkResult = vkEnumerateInstanceLayerProperties(&validationLayerCount, vkLayerProperties);

    if (XR_IS_ERROR(vkResult))
    {
        XR_FREE(vkLayerProperties);
        validationLayerFound = VK_FALSE;
        return validationLayerFound;
    }

    for (uint32_t index = 0; index < validationLayerCount; ++index)
    {
        XR_LOG_INFO(context->logger, "Validation layer name [%d]: %s", index, vkLayerProperties[index].layerName);
    }

    for (uint32_t index = 0; index < validationLayerCount; ++index)
    {
        if (strcmp(vkLayerProperties[index].layerName, validationLayerName) == 0)
        {
            validationLayerFound = VK_TRUE;
            break;
        }
    }

    XR_FREE(vkLayerProperties);

    return validationLayerFound;
}

XR_API void xrWaitForIdle(XrContext *context)
{
    vkDeviceWaitIdle(context->device);
}

XR_API uint32_t xrFindMemoryTypeIndex(
    VkPhysicalDeviceMemoryProperties *gpuMemoryProperties,
    VkMemoryRequirements *imageMemoryRequirements,
    VkMemoryPropertyFlags requiredMemoryProperties
)
{
    for (uint32_t memoryTypeCounter = 0; memoryTypeCounter < gpuMemoryProperties->memoryTypeCount; ++memoryTypeCounter)
    {
        if (imageMemoryRequirements->memoryTypeBits & (1 << memoryTypeCounter))
        {
            if ((gpuMemoryProperties->memoryTypes[memoryTypeCounter].propertyFlags & requiredMemoryProperties) == requiredMemoryProperties)
            {
                return memoryTypeCounter;
            }
        }
    }

    return UINT32_MAX;
}

XR_API VkResult xrInitLogicalDevice(XrContext *context)
{
    float queuePriorities[1] = {0.0f};
    uint32_t queueCount = context->gpu->hasSeparatePresentQueue ? 2 : 1;

    VkDeviceQueueCreateInfo *deviceQueueCreateInfos = (VkDeviceQueueCreateInfo *)malloc(sizeof(VkDeviceQueueCreateInfo) * queueCount);
    memset((void *)deviceQueueCreateInfos, 0, sizeof(VkDeviceQueueCreateInfo) * queueCount);

    deviceQueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfos[0].pNext = VK_NULL_HANDLE;
    deviceQueueCreateInfos[0].flags = 0;
    deviceQueueCreateInfos[0].queueFamilyIndex = context->gpu->graphicsFamilyIndex;
    deviceQueueCreateInfos[0].queueCount = 1;
    deviceQueueCreateInfos[0].pQueuePriorities = queuePriorities;

    if (context->gpu->hasSeparatePresentQueue)
    {
        deviceQueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCreateInfos[1].pNext = VK_NULL_HANDLE;
        deviceQueueCreateInfos[1].flags = 0;
        deviceQueueCreateInfos[1].queueFamilyIndex = context->gpu->presentFamilyIndex;
        deviceQueueCreateInfos[1].queueCount = 1;
        deviceQueueCreateInfos[1].pQueuePriorities = queuePriorities;
    }

    // As we are using texture sampler, we need to enable this as a device feature.
    // This have many VkBool32 properties, leave it to VK_FALSE right now.
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = VK_NULL_HANDLE;
    deviceCreateInfo.flags = 0;
    deviceCreateInfo.queueCreateInfoCount = queueCount;
    deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos;
    deviceCreateInfo.enabledExtensionCount = context->deviceExtensionsCount;
    deviceCreateInfo.ppEnabledExtensionNames = context->deviceExtensions;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    VkResult vkResult = vkCreateDevice(context->gpu->gpu, &deviceCreateInfo, VK_NULL_HANDLE, &(context->device));

    if (XR_IS_ERROR(vkResult))
    {
        XR_FREE(deviceQueueCreateInfos);
        return vkResult;
    }

    // Create the graphic queue using graphicsFamilyIndex for given physical device.
    vkGetDeviceQueue(context->device, context->gpu->graphicsFamilyIndex, 0, &(context->graphicsQueue));

    if (!context->gpu->hasSeparatePresentQueue)
    {
        context->presentQueue = context->graphicsQueue;
    }
    else
    {
        vkGetDeviceQueue(context->device, context->gpu->presentFamilyIndex, 0, &(context->presentQueue));
    }

    XR_FREE(deviceQueueCreateInfos);

    return vkResult;
}

XR_API void xrDestroyDevice(XrContext *context)
{
    vkDestroyDevice(context->device, VK_NULL_HANDLE);
    context->device = VK_NULL_HANDLE;
}

VkResult xrQuerySwapchainSupportDetails(XrContext *context, VkPhysicalDevice gpu, XrSwapchainSupport *details)
{
    VkResult vkResult = VK_SUCCESS;

    vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, context->surface, &(details->surfaceCapabilities));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, context->surface, &details->surfaceFormatsCount, VK_NULL_HANDLE);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    details->surfaceFormats = (VkSurfaceFormatKHR *)malloc(sizeof(VkSurfaceFormatKHR) * details->surfaceFormatsCount);
    memset((void *)details->surfaceFormats, 0, sizeof(VkSurfaceFormatKHR) * details->surfaceFormatsCount);

    vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, context->surface, &details->surfaceFormatsCount, details->surfaceFormats);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, context->surface, &details->presentModesCount, VK_NULL_HANDLE);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    details->presentModes = (VkPresentModeKHR *)malloc(sizeof(VkPresentModeKHR) * details->presentModesCount);
    memset((void *)details->presentModes, 0, sizeof(VkPresentModeKHR) * details->presentModesCount);

    vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, context->surface, &details->presentModesCount, details->presentModes);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

XR_API void xrChooseSurfaceFormat(VkSurfaceFormatKHR *surfaceFormats, uint32_t surfaceFormatsCount, VkSurfaceFormatKHR *surfaceFormat)
{
    if (surfaceFormatsCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        surfaceFormat->format = VK_FORMAT_B8G8R8A8_SRGB;
        surfaceFormat->colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        return;
    }

    for (uint32_t counter = 0; counter < surfaceFormatsCount; ++counter)
    {
        VkSurfaceFormatKHR nextSurfaceFormat = surfaceFormats[counter];

        if (nextSurfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && nextSurfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            surfaceFormat->format = nextSurfaceFormat.format;
            surfaceFormat->colorSpace = nextSurfaceFormat.colorSpace;
            return;
        }
    }

    surfaceFormat->format = surfaceFormats[0].format;
    surfaceFormat->colorSpace = surfaceFormats[0].colorSpace;
}

XR_API void xrChoosePresentMode(XrContext *context, VkPresentModeKHR *presentModes, uint32_t presentModesCount, VkPresentModeKHR *presentMode)
{
    for (uint32_t counter = 0; counter < presentModesCount; ++counter)
    {
        VkPresentModeKHR nextPresentMode = presentModes[counter];

        // If nextPresentMode is VK_PRESENT_MODE_MAILBOX_KHR then use this as this is the best.
        if (nextPresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            *presentMode = nextPresentMode;
            return;
        }
    }

    for (uint32_t counter = 0; counter < presentModesCount; ++counter)
    {
        VkPresentModeKHR nextPresentMode = presentModes[counter];

        // If VK_PRESENT_MODE_MAILBOX_KHR was not found then use VK_PRESENT_MODE_IMMEDIATE_KHR.
        if (nextPresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            *presentMode = nextPresentMode;
            return;
        }
    }

    *presentMode = VK_PRESENT_MODE_FIFO_KHR;
}

XR_API void xrChooseSurfaceExtent(VkSurfaceCapabilitiesKHR surfaceCapabilities, VkExtent2D *surfaceExtent)
{
    if (surfaceCapabilities.currentExtent.width < UINT32_MAX)
    {
        surfaceExtent->width = surfaceCapabilities.currentExtent.width;
        surfaceExtent->height = surfaceCapabilities.currentExtent.height;
    }
    else
    {
        if (surfaceExtent->width > surfaceCapabilities.maxImageExtent.width)
        {
            surfaceExtent->width = surfaceCapabilities.maxImageExtent.width;
        }

        if (surfaceExtent->width < surfaceCapabilities.minImageExtent.width)
        {
            surfaceExtent->width = surfaceCapabilities.minImageExtent.width;
        }

        if (surfaceExtent->height > surfaceCapabilities.maxImageExtent.height)
        {
            surfaceExtent->height = surfaceCapabilities.maxImageExtent.height;
        }

        if (surfaceExtent->height < surfaceCapabilities.minImageExtent.height)
        {
            surfaceExtent->height = surfaceCapabilities.minImageExtent.height;
        }
    }
}

XR_API VkResult xrInitSwapchain(XrContext *context)
{
    context->swapchainSupport = (XrSwapchainSupport *)malloc(sizeof(XrSwapchainSupport));
    memset((void *)context->swapchainSupport, 0, sizeof(XrSwapchainSupport));

    xrQuerySwapchainSupportDetails(context, context->gpu->gpu, context->swapchainSupport);

    if (!context->swapchainSupport->surfaceFormatsCount)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    xrPrintSurfaceFormatsDetails(context, context->swapchainSupport->surfaceFormats, context->swapchainSupport->surfaceFormatsCount);

    xrChooseSurfaceFormat(context->swapchainSupport->surfaceFormats, context->swapchainSupport->surfaceFormatsCount, &(context->surfaceFormat));

    xrChooseSurfaceExtent(context->swapchainSupport->surfaceCapabilities, &(context->surfaceExtent));

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    xrChoosePresentMode(context, context->swapchainSupport->presentModes, context->swapchainSupport->presentModesCount, &presentMode);

    // surfaceCapabilities.maxImageCount can be 0.
    // In this case the implementation supports unlimited amount of swap-chain images, limited by memory.
    // The amount of swap-chain images can also be fixed.

    uint32_t imageCount = context->swapchainSupport->surfaceCapabilities.minImageCount + 1;

    if (context->swapchainSupport->surfaceCapabilities.maxImageCount > 0 && imageCount > context->swapchainSupport->surfaceCapabilities.maxImageCount)
    {
        imageCount = context->swapchainSupport->surfaceCapabilities.maxImageCount;
    }

    context->swapchainImageCount = imageCount;

    xrPrintSwapChainImageCount(
        context,
        context->swapchainSupport->surfaceCapabilities.minImageCount,
        context->swapchainSupport->surfaceCapabilities.maxImageCount,
        context->swapchainImageCount
    );

    {
        XR_LOG_INFO(context->logger, "---------- Presentation Mode ----------");

        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            XR_LOG_INFO(context->logger, "Mode: MAILBOX [%d]", presentMode);
        }
        else
        {
            XR_LOG_INFO(context->logger, "Mode: %d", presentMode);
        }

        XR_LOG_INFO(context->logger, "---------- Presentation Mode End ----------");
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = VK_NULL_HANDLE;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = context->surface;
    swapchainCreateInfo.minImageCount = context->swapchainImageCount;
    swapchainCreateInfo.imageFormat = context->surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = context->surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent.width = context->surfaceExtent.width;
    swapchainCreateInfo.imageExtent.height = context->surfaceExtent.height;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.preTransform = context->swapchainSupport->surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    if (context->gpu->hasSeparatePresentQueue)
    {
        uint32_t indices[2] = {context->gpu->graphicsFamilyIndex, context->gpu->presentFamilyIndex};

        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = _ARRAYSIZE(indices); // Ignored if imageSharingMode is VK_SHARING_MODE_EXCLUSIVE
        swapchainCreateInfo.pQueueFamilyIndices = indices;
    }
    else
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;            // Ignored if imageSharingMode is VK_SHARING_MODE_EXCLUSIVE
        swapchainCreateInfo.pQueueFamilyIndices = VK_NULL_HANDLE; // Ignored if imageSharingMode is VK_SHARING_MODE_EXCLUSIVE
    }

    VkResult vkResult = vkCreateSwapchainKHR(context->device, &swapchainCreateInfo, VK_NULL_HANDLE, &(context->swapchain));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = vkGetSwapchainImagesKHR(context->device, context->swapchain, &(context->swapchainImageCount), VK_NULL_HANDLE);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    context->swapchainImages = (VkImage *)malloc(sizeof(VkImage) * context->swapchainImageCount);
    memset((void *)context->swapchainImages, 0, sizeof(VkImage) * context->swapchainImageCount);

    vkResult = vkGetSwapchainImagesKHR(context->device, context->swapchain, &(context->swapchainImageCount), context->swapchainImages);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

XR_API void xrDestroySwapchain(XrContext *context)
{
    XR_FREE(context->swapchainImages)

    if (context->swapchain)
    {
        vkDestroySwapchainKHR(context->device, context->swapchain, VK_NULL_HANDLE);
        context->swapchain = VK_NULL_HANDLE;
    }

    if (context->swapchainSupport)
    {
        XR_FREE(context->swapchainSupport->surfaceFormats);
        XR_FREE(context->swapchainSupport->presentModes);
        XR_FREE(context->swapchainSupport);
    }
}

XR_API VkResult xrInitSwapchainImageViews(XrContext *context)
{
    VkResult vkResult = VK_SUCCESS;

    context->swapchainImageViews = (VkImageView *)malloc(sizeof(VkImageView) * context->swapchainImageCount);
    memset((void *)context->swapchainImageViews, 0, sizeof(VkImageView) * context->swapchainImageCount);

    for (uint32_t counter = 0; counter < context->swapchainImageCount; ++counter)
    {
        vkResult = xrCreateImageView(
            context, context->swapchainImages[counter], context->surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, &(context->swapchainImageViews[counter])
        );

        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }
    }

    return vkResult;
}

XR_API void xrDestroySwapchainImageViews(XrContext *context)
{
    for (uint32_t counter = 0; counter < context->swapchainImageCount; ++counter)
    {
        if (context->swapchainImageViews[counter])
        {
            vkDestroyImageView(context->device, context->swapchainImageViews[counter], VK_NULL_HANDLE);
            context->swapchainImageViews[counter] = VK_NULL_HANDLE;
        }
    }

    XR_FREE(context->swapchainImageViews);
}

XR_API VkResult xrCreateShaderModule(XrContext *context, const char *shaderFilePath, VkShaderModule *shaderModule)
{
    VkResult vkResult = VK_SUCCESS;

    char *shaderCode = VK_NULL_HANDLE;
    size_t codeSize = 0;

    if (!xrReadFile(shaderFilePath, &shaderCode, &codeSize))
    {
        XR_FREE(shaderCode);
        XR_LOG_ERROR(context->logger, "Failed to get shader code");
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return vkResult;
    }

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pNext = VK_NULL_HANDLE;
    shaderModuleCreateInfo.flags = 0;
    shaderModuleCreateInfo.codeSize = codeSize;
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(shaderCode);

    vkResult = vkCreateShaderModule(context->device, &shaderModuleCreateInfo, VK_NULL_HANDLE, shaderModule);

    XR_FREE(shaderCode);

    return vkResult;
}

XR_API void xrDestroyShaderModule(XrContext *context, VkShaderModule *shaderModule)
{
    if (*shaderModule)
    {
        vkDestroyShaderModule(context->device, *shaderModule, VK_NULL_HANDLE);
        *shaderModule = VK_NULL_HANDLE;
    }
}

XR_API VkResult xrInitGraphicsPiplineCache(XrContext *context)
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCacheCreateInfo.pNext = NULL;
    pipelineCacheCreateInfo.flags = 0;
    pipelineCacheCreateInfo.initialDataSize = 0;
    pipelineCacheCreateInfo.pInitialData = NULL;

    return vkCreatePipelineCache(context->device, &pipelineCacheCreateInfo, VK_NULL_HANDLE, &(context->pipelineCache));
}

XR_API void xrDestroyGraphicsPiplineCache(XrContext *context)
{
    if (context->pipelineCache)
    {
        vkDestroyPipelineCache(context->device, context->pipelineCache, VK_NULL_HANDLE);
        context->pipelineCache = VK_NULL_HANDLE;
    }
}

XR_API VkResult xrInitGraphicsPipline(XrContext *context)
{
    VkResult vkResult = VK_SUCCESS;

    VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {};
    vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageCreateInfo.pNext = VK_NULL_HANDLE;
    vertexShaderStageCreateInfo.flags = 0;
    vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageCreateInfo.module = context->vertexShaderModule;
    vertexShaderStageCreateInfo.pName = "main";
    vertexShaderStageCreateInfo.pSpecializationInfo = VK_NULL_HANDLE;

    VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {};
    fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageCreateInfo.pNext = VK_NULL_HANDLE;
    fragmentShaderStageCreateInfo.flags = 0;
    fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStageCreateInfo.module = context->fragmentShaderModule;
    fragmentShaderStageCreateInfo.pName = "main";
    fragmentShaderStageCreateInfo.pSpecializationInfo = VK_NULL_HANDLE;

    VkPipelineShaderStageCreateInfo shaderStageCreateInfos[2] = {vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo};

    VkVertexInputBindingDescription vertexBindingDescription = {};
    xrGetBindingDescription(&vertexBindingDescription);

    VkVertexInputAttributeDescription *vertexAttributeDescription = VK_NULL_HANDLE;
    uint32_t vertexAttributeDescriptionCount = 0;

    xrGetAttributeDescriptions(&vertexAttributeDescription, &vertexAttributeDescriptionCount);

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.pNext = VK_NULL_HANDLE;
    vertexInputStateCreateInfo.flags = 0;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptionCount;
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescription;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
    inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.pNext = VK_NULL_HANDLE;
    inputAssemblyStateCreateInfo.flags = 0;
    inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)(context->surfaceExtent.width);
    viewport.height = (float)(context->surfaceExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent.width = context->surfaceExtent.width;
    scissor.extent.height = context->surfaceExtent.height;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.pNext = VK_NULL_HANDLE;
    viewportStateCreateInfo.flags = 0;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.pNext = VK_NULL_HANDLE;
    rasterizationStateCreateInfo.flags = 0;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
    rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationStateCreateInfo.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.pNext = VK_NULL_HANDLE;
    multisampleStateCreateInfo.flags = 0;
    multisampleStateCreateInfo.rasterizationSamples = context->gpu->msaaSamples;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.minSampleShading = 1.0f;
    multisampleStateCreateInfo.pSampleMask = VK_NULL_HANDLE;
    multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
    depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCreateInfo.pNext = VK_NULL_HANDLE;
    depthStencilStateCreateInfo.flags = 0;
    depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.front.failOp = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.front.passOp = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.front.compareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilStateCreateInfo.back.failOp = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.back.passOp = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendingStateCreateInfo = {};
    colorBlendingStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingStateCreateInfo.pNext = VK_NULL_HANDLE;
    colorBlendingStateCreateInfo.flags = 0;
    colorBlendingStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendingStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendingStateCreateInfo.attachmentCount = 1;
    colorBlendingStateCreateInfo.pAttachments = &colorBlendAttachment;
    colorBlendingStateCreateInfo.blendConstants[0] = 0.0f;
    colorBlendingStateCreateInfo.blendConstants[1] = 0.0f;
    colorBlendingStateCreateInfo.blendConstants[2] = 0.0f;
    colorBlendingStateCreateInfo.blendConstants[3] = 0.0f;

    // We are not yet using the dynamic state hence removing this for now.
    // VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };
    VkDynamicState *dynamicStates = VK_NULL_HANDLE;
    uint32_t dynamicStatesCount = 0;

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.pNext = VK_NULL_HANDLE;
    dynamicStateCreateInfo.flags = 0;
    dynamicStateCreateInfo.dynamicStateCount = dynamicStatesCount;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = VK_NULL_HANDLE;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &(context->descriptorSetLayout);
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = 0;

    vkResult = vkCreatePipelineLayout(context->device, &pipelineLayoutCreateInfo, VK_NULL_HANDLE, &(context->pipelineLayout));
    if (XR_IS_ERROR(vkResult))
    {
        XR_FREE(vertexAttributeDescription);
        return vkResult;
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext = VK_NULL_HANDLE;
    pipelineCreateInfo.flags = 0;
    pipelineCreateInfo.stageCount = _ARRAYSIZE(shaderStageCreateInfos);
    pipelineCreateInfo.pStages = shaderStageCreateInfos;
    pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    pipelineCreateInfo.pTessellationState = VK_NULL_HANDLE;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendingStateCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = context->pipelineLayout;
    pipelineCreateInfo.renderPass = context->renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    vkResult = vkCreateGraphicsPipelines(context->device, context->pipelineCache, 1, &pipelineCreateInfo, VK_NULL_HANDLE, &(context->pipeline));
    if (XR_IS_ERROR(vkResult))
    {
        XR_FREE(vertexAttributeDescription);
        return vkResult;
    }

    XR_FREE(vertexAttributeDescription);

    return vkResult;
}

XR_API void xrDestroyGraphicsPipline(XrContext *context)
{
    if (context->pipeline)
    {
        vkDestroyPipeline(context->device, context->pipeline, VK_NULL_HANDLE);
        context->pipeline = VK_NULL_HANDLE;
    }

    if (context->pipelineLayout)
    {
        vkDestroyPipelineLayout(context->device, context->pipelineLayout, VK_NULL_HANDLE);
        context->pipelineLayout = VK_NULL_HANDLE;
    }
}

XR_API void xrFindSupportedFormat(
    VkPhysicalDevice gpu,
    VkFormat *formats,
    uint32_t formatsCount,
    VkImageTiling imageTiling,
    VkFormatFeatureFlags formatFeatureFlags,
    VkFormat *supportedFormat
)
{
    for (uint32_t counter = 0; counter < formatsCount; ++counter)
    {
        VkFormat nextFormat = formats[counter];
        VkFormatProperties formatProperties = {};
        vkGetPhysicalDeviceFormatProperties(gpu, nextFormat, &formatProperties);

        if (imageTiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & formatFeatureFlags) == formatFeatureFlags)
        {
            *supportedFormat = nextFormat;
            return;
        }

        if (imageTiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & formatFeatureFlags) == formatFeatureFlags)
        {
            *supportedFormat = nextFormat;
            return;
        }
    }

    *supportedFormat = VK_FORMAT_UNDEFINED;
}

XR_API void xrFindDepthFormat(XrContext *context, VkFormat *format)
{
    VkFormat formats[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    xrFindSupportedFormat(context->gpu->gpu, formats, _ARRAYSIZE(formats), VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, format);
}

XR_API VkBool32 xrHasStencilComponent(VkFormat format)
{
    if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
    {
        return VK_TRUE;
    }

    return VK_FALSE;
}

XR_API VkResult xrInitDepthStencilImage(XrContext *context)
{
    VkResult vkResult = VK_SUCCESS;

    context->depthStencilFormat = VK_FORMAT_UNDEFINED;

    xrFindDepthFormat(context, &context->depthStencilFormat);

    if (context->depthStencilFormat == VK_FORMAT_UNDEFINED)
    {
        XR_LOG_ERROR(context->logger, "Depth format not found");
        vkResult = VK_ERROR_FORMAT_NOT_SUPPORTED;
        return vkResult;
    }

    context->depthImage = (XrImage *)malloc(sizeof(XrImage));
    memset((void *)context->depthImage, 0, sizeof(XrImage));

    vkResult = xrCreateImage(
        context,
        &context->surfaceExtent,
        1,
        context->gpu->msaaSamples,
        context->depthStencilFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &(context->depthImage->image),
        &(context->depthImage->memory)
    );

    if (XR_IS_ERROR(vkResult))
    {
        XR_LOG_ERROR(context->logger, "Failed to create depth image");
        return vkResult;
    }

    vkResult =
        xrCreateImageView(context, context->depthImage->image, context->depthStencilFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, &(context->depthImage->imageView));

    if (XR_IS_ERROR(vkResult))
    {
        XR_LOG_ERROR(context->logger, "Failed to create depth image view");
        return vkResult;
    }

    return vkResult;
}

XR_API void xrDestroyDepthStencilImage(XrContext *context)
{
    if (context->depthImage)
    {
        if (context->depthImage->imageView)
        {
            vkDestroyImageView(context->device, context->depthImage->imageView, VK_NULL_HANDLE);
            context->depthImage->imageView = VK_NULL_HANDLE;
        }

        if (context->depthImage->memory)
        {
            vkFreeMemory(context->device, context->depthImage->memory, VK_NULL_HANDLE);
            context->depthImage->memory = VK_NULL_HANDLE;
        }

        if (context->depthImage->image)
        {
            vkDestroyImage(context->device, context->depthImage->image, VK_NULL_HANDLE);
            context->depthImage->image = VK_NULL_HANDLE;
        }
    }

    XR_FREE(context->depthImage);
}

XR_API VkResult xrInitMSAAColorImage(XrContext *context)
{
    VkResult vkResult = VK_SUCCESS;

    context->msaaColorImage = (XrImage *)malloc(sizeof(XrImage));
    memset((void *)context->msaaColorImage, 0, sizeof(XrImage));

    vkResult = xrCreateImage(
        context,
        &context->surfaceExtent,
        1,
        context->gpu->msaaSamples,
        context->surfaceFormat.format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &(context->msaaColorImage->image),
        &(context->msaaColorImage->memory)
    );

    if (XR_IS_ERROR(vkResult))
    {
        XR_LOG_ERROR(context->logger, "Failed to create MSAA color image");
        return vkResult;
    }

    vkResult = xrCreateImageView(
        context, context->msaaColorImage->image, context->surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1, &(context->msaaColorImage->imageView)
    );

    if (XR_IS_ERROR(vkResult))
    {
        XR_LOG_ERROR(context->logger, "Failed to create MSAA color image view");
        return vkResult;
    }

    return vkResult;
}

XR_API void xrDestroyMSAAColorImage(XrContext *context)
{
    if (context->msaaColorImage)
    {
        if (context->msaaColorImage->imageView)
        {
            vkDestroyImageView(context->device, context->msaaColorImage->imageView, VK_NULL_HANDLE);
            context->msaaColorImage->imageView = VK_NULL_HANDLE;
        }

        if (context->msaaColorImage->memory)
        {
            vkFreeMemory(context->device, context->msaaColorImage->memory, VK_NULL_HANDLE);
            context->msaaColorImage->memory = VK_NULL_HANDLE;
        }

        if (context->msaaColorImage->image)
        {
            vkDestroyImage(context->device, context->msaaColorImage->image, VK_NULL_HANDLE);
            context->msaaColorImage->image = VK_NULL_HANDLE;
        }
    }

    XR_FREE(context->msaaColorImage)
}

XR_API VkResult xrInitRenderPass(XrContext *context)
{
    VkAttachmentDescription colorAttachmentDescription = {};
    colorAttachmentDescription.flags = 0;
    colorAttachmentDescription.format = context->surfaceFormat.format;
    colorAttachmentDescription.samples = context->gpu->msaaSamples;
    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthStencilAttachmentDescription = {};
    depthStencilAttachmentDescription.flags = 0;
    depthStencilAttachmentDescription.format = context->depthStencilFormat;
    depthStencilAttachmentDescription.samples = context->gpu->msaaSamples;
    depthStencilAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthStencilAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthStencilAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthStencilAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthStencilAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthStencilAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolveDescription = {};
    colorAttachmentResolveDescription.flags = 0;
    colorAttachmentResolveDescription.format = context->surfaceFormat.format;
    colorAttachmentResolveDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolveDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolveDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolveDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolveDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolveDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolveDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthStencilAttachmentReference = {};
    depthStencilAttachmentReference.attachment = 1;
    depthStencilAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveReference = {};
    colorAttachmentResolveReference.attachment = 2;
    colorAttachmentResolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.flags = 0;
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = VK_NULL_HANDLE;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;
    subpassDescription.pResolveAttachments = &colorAttachmentResolveReference;
    subpassDescription.pDepthStencilAttachment = &depthStencilAttachmentReference;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = VK_NULL_HANDLE;

    VkAttachmentDescription attachments[3] = {colorAttachmentDescription, depthStencilAttachmentDescription, colorAttachmentResolveDescription};

    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = VK_NULL_HANDLE;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = _ARRAYSIZE(attachments);
    renderPassCreateInfo.pAttachments = attachments;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    return vkCreateRenderPass(context->device, &renderPassCreateInfo, VK_NULL_HANDLE, &(context->renderPass));
}

XR_API void xrDestroyRenderPass(XrContext *context)
{
    if (context->renderPass)
    {
        vkDestroyRenderPass(context->device, context->renderPass, VK_NULL_HANDLE);
        context->renderPass = VK_NULL_HANDLE;
    }
}

XR_API VkResult xrInitDescriptorSetLayout(XrContext *context)
{
    VkDescriptorSetLayoutBinding uboDescriptorSetLayoutBinding = {};
    uboDescriptorSetLayoutBinding.binding = 0;
    uboDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboDescriptorSetLayoutBinding.descriptorCount = 1;
    uboDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboDescriptorSetLayoutBinding.pImmutableSamplers = VK_NULL_HANDLE;

    VkDescriptorSetLayoutBinding samplerDescriptorSetLayoutBinding = {};
    samplerDescriptorSetLayoutBinding.binding = 1;
    samplerDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerDescriptorSetLayoutBinding.descriptorCount = 1;
    samplerDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerDescriptorSetLayoutBinding.pImmutableSamplers = VK_NULL_HANDLE;

    VkDescriptorSetLayoutBinding layoutBindings[2] = {uboDescriptorSetLayoutBinding, samplerDescriptorSetLayoutBinding};

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = VK_NULL_HANDLE;
    descriptorSetLayoutCreateInfo.flags = 0;
    descriptorSetLayoutCreateInfo.bindingCount = _ARRAYSIZE(layoutBindings);
    descriptorSetLayoutCreateInfo.pBindings = layoutBindings;

    return vkCreateDescriptorSetLayout(context->device, &descriptorSetLayoutCreateInfo, VK_NULL_HANDLE, &(context->descriptorSetLayout));
}

XR_API void xrDestroyDescriptorSetLayout(XrContext *context)
{
    if (context->descriptorSetLayout)
    {
        vkDestroyDescriptorSetLayout(context->device, context->descriptorSetLayout, VK_NULL_HANDLE);
        context->descriptorSetLayout = VK_NULL_HANDLE;
    }
}

XR_API VkResult xrInitFrameBuffers(XrContext *context)
{
    VkResult vkResult = VK_SUCCESS;
    context->framebuffers = (VkFramebuffer *)malloc(sizeof(VkFramebuffer) * context->swapchainImageCount);
    memset((void *)context->framebuffers, 0, sizeof(VkFramebuffer) * context->swapchainImageCount);

    for (uint32_t swapchainImageCounter = 0; swapchainImageCounter < context->swapchainImageCount; ++swapchainImageCounter)
    {
        VkImageView attachments[3] = {context->msaaColorImage->imageView, context->depthImage->imageView, context->swapchainImageViews[swapchainImageCounter]};

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = VK_NULL_HANDLE;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = context->renderPass;
        framebufferCreateInfo.attachmentCount = _ARRAYSIZE(attachments);
        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = context->surfaceExtent.width;
        framebufferCreateInfo.height = context->surfaceExtent.height;
        framebufferCreateInfo.layers = 1;

        vkResult = vkCreateFramebuffer(context->device, &framebufferCreateInfo, VK_NULL_HANDLE, &(context->framebuffers[swapchainImageCounter]));
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }
    }

    return vkResult;
}

XR_API void xrDestroyFrameBuffers(XrContext *context)
{
    if (context->framebuffers)
    {
        for (uint32_t swapchainImageCounter = 0; swapchainImageCounter < context->swapchainImageCount; ++swapchainImageCounter)
        {
            if (context->framebuffers[swapchainImageCounter])
            {
                vkDestroyFramebuffer(context->device, context->framebuffers[swapchainImageCounter], VK_NULL_HANDLE);
                context->framebuffers[swapchainImageCounter] = VK_NULL_HANDLE;
            }
        }
    }

    XR_FREE(context->framebuffers);
}

XR_API VkResult xrInitCommandPool(XrContext *context)
{
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = VK_NULL_HANDLE;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = context->gpu->graphicsFamilyIndex;

    return vkCreateCommandPool(context->device, &commandPoolCreateInfo, VK_NULL_HANDLE, &(context->commandPool));
}

XR_API void xrDestroyCommandPool(XrContext *context)
{
    if (context->commandPool)
    {
        vkDestroyCommandPool(context->device, context->commandPool, VK_NULL_HANDLE);
        context->commandPool = VK_NULL_HANDLE;
    }
}

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
)
{
    VkResult vkResult = VK_SUCCESS;

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = VK_NULL_HANDLE;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent.width = imageExtent->width;
    imageCreateInfo.extent.height = imageExtent->height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = samplesCount;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.usage = usage;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0;
    imageCreateInfo.pQueueFamilyIndices = VK_NULL_HANDLE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    vkResult = vkCreateImage(context->device, &imageCreateInfo, VK_NULL_HANDLE, image);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkMemoryRequirements imageMemoryRequirements = {};
    vkGetImageMemoryRequirements(context->device, *image, &imageMemoryRequirements);

    uint32_t memoryIndex = xrFindMemoryTypeIndex(&(context->gpu->memoryProperties), &imageMemoryRequirements, memoryPropertyFlags);

    VkMemoryAllocateInfo memoryAllocationInfo = {};
    memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocationInfo.pNext = VK_NULL_HANDLE;
    memoryAllocationInfo.allocationSize = imageMemoryRequirements.size;
    memoryAllocationInfo.memoryTypeIndex = memoryIndex;

    vkResult = vkAllocateMemory(context->device, &memoryAllocationInfo, VK_NULL_HANDLE, imageMemory);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = vkBindImageMemory(context->device, *image, *imageMemory, 0);

    return vkResult;
}

XR_API VkResult
xrCreateImageView(XrContext *context, VkImage image, VkFormat format, VkImageAspectFlags imageAspectFlags, uint32_t mipLevels, VkImageView *imageView)
{
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = VK_NULL_HANDLE;
    imageViewCreateInfo.flags = 0;
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = format;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange.aspectMask = imageAspectFlags;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = mipLevels;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    return vkCreateImageView(context->device, &imageViewCreateInfo, VK_NULL_HANDLE, imageView);
}

XR_API VkResult xrInitTextureImage(XrContext *context, XrTexture *texture, void *pixels)
{
    VkResult vkResult = VK_SUCCESS;
    XrBuffer stagingImageBuffer = {};
    stagingImageBuffer.memorySize = texture->extent.width * texture->extent.height * 4;

    vkResult = xrCreateBuffer(
        context, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingImageBuffer
    );

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    texture->image = (XrImage *)malloc(sizeof(XrImage));
    memset((void *)texture->image, 0, sizeof(XrImage));

    void *data = VK_NULL_HANDLE;
    vkMapMemory(context->device, stagingImageBuffer.memory, 0, stagingImageBuffer.memorySize, 0, &data);
    memcpy(data, pixels, stagingImageBuffer.memorySize);
    vkUnmapMemory(context->device, stagingImageBuffer.memory);

    vkResult = xrCreateImage(
        context,
        &(texture->extent),
        texture->mipLevels,
        VK_SAMPLE_COUNT_1_BIT,
        texture->format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &(texture->image->image),
        &(texture->image->memory)
    );

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrTransitionImageLayout(
        context, &(texture->image->image), texture->format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture->mipLevels
    );

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrCopyBufferToImage(context, stagingImageBuffer.buffer, texture->image->image, &(texture->extent));

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    // Generate the mipmaps images and then transition image layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
    vkResult = xrGenerateMipmaps(context, &(texture->image->image), texture);

    xrDestroyBuffer(context, &stagingImageBuffer);

    return vkResult;
}

XR_API void xrDestroyTextureImage(XrContext *context, XrTexture *texture)
{
    if (texture && texture->image)
    {
        if (texture->image->image)
        {
            vkDestroyImage(context->device, texture->image->image, VK_NULL_HANDLE);
            texture->image->image = VK_NULL_HANDLE;
        }

        if (texture->image->memory)
        {
            vkFreeMemory(context->device, texture->image->memory, VK_NULL_HANDLE);
            texture->image->memory = VK_NULL_HANDLE;
        }
    }
}

VkResult xrGenerateMipmaps(XrContext *context, VkImage *image, XrTexture *texture)
{
    VkResult vkResult = VK_SUCCESS;
    uint32_t mipWidth = texture->extent.width;
    uint32_t mipHeight = texture->extent.height;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkResult = xrBeginOneTimeCommand(context, &commandBuffer);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = VK_NULL_HANDLE;
    imageMemoryBarrier.image = *image;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;
    imageMemoryBarrier.subresourceRange.levelCount = 1;

    // mip level starts from 1 not 0.
    for (uint32_t counter = 1; counter < texture->mipLevels; ++counter)
    {
        // First, we transition level counter - 1 to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL.
        // This transition will wait for level counter - 1 to be filled, either from the previous blit command,
        // or from vkCmdCopyBufferToImage
        imageMemoryBarrier.subresourceRange.baseMipLevel = counter - 1;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageMemoryBarrier
        );

        VkImageBlit imageBlit = {};

        // srcOffsets array determine the 3D region that data will be blitted from.
        imageBlit.srcOffsets[0] = {};
        imageBlit.srcOffsets[0].x = 0;
        imageBlit.srcOffsets[0].y = 0;
        imageBlit.srcOffsets[0].z = 0;

        // The z dimension of srcOffsets[1] must be 1, since a 2D image has a depth of 1
        imageBlit.srcOffsets[1] = {};
        imageBlit.srcOffsets[1].x = mipWidth;
        imageBlit.srcOffsets[1].y = mipHeight;
        imageBlit.srcOffsets[1].z = 1;

        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        // The source mip level is counter - 1
        imageBlit.srcSubresource.mipLevel = counter - 1;
        imageBlit.srcSubresource.baseArrayLayer = 0;
        imageBlit.srcSubresource.layerCount = 1;

        // dstOffsets determines the region that data will be blitted to.
        imageBlit.dstOffsets[0] = {0, 0, 0};

        // The x and y dimensions of the dstOffsets[1] are divided by two since
        // each mip level is half the size of the previous level.
        // The z dimension of dstOffsets[1] must be 1, since a 2D image has a depth of 1
        // In case where we have odd texture dimensions, the mip dimension may reach 1.
        // this will cause 0 to be passed to VkImageBlit.dstOffsets which results in validation layer warning.
        // To avoid this, check is next mip level is non-zero, if it is, then use 1 instead of 0.
        imageBlit.dstOffsets[1] = {};
        imageBlit.dstOffsets[1].x = mipWidth > 1 ? mipWidth / 2 : 1;
        imageBlit.dstOffsets[1].y = mipHeight > 1 ? mipHeight / 2 : 1;
        imageBlit.dstOffsets[1].z = 1;
        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        // The destination mip level is counter
        imageBlit.dstSubresource.mipLevel = counter;
        imageBlit.dstSubresource.baseArrayLayer = 0;
        imageBlit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(
            commandBuffer, *image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR
        );

        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        // This barrier transitions mip level i - 1 to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
        // This transition waits on the current blit command to finish.
        // All sampling operations will wait on this transition to finish.
        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            VK_NULL_HANDLE,
            0,
            VK_NULL_HANDLE,
            1,
            &imageMemoryBarrier
        );

        // Divide the current mip dimensions by 2
        // Check each dimension before the division to ensure that dimension never becomes 0.
        // This handles cases where the image is not square, since one of the mip dimensions would reach 1 before the other dimension.
        // When this happens, that dimension should remain 1 for all remaining levels.
        if (mipWidth > 1)
        {
            mipWidth = mipWidth / 2;
        }

        if (mipHeight > 1)
        {
            mipHeight = mipHeight / 2;
        }
    }

    // Before we end the command buffer, we insert one more pipeline barrier.
    // This barrier transitions the last mip level from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
    // This wasn't handled by the loop, since the last mip level is never blitted from.
    imageMemoryBarrier.subresourceRange.baseMipLevel = texture->mipLevels - 1;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageMemoryBarrier
    );

    // Now end the command buffer.
    vkResult = xrEndOneTimeCommand(context, &commandBuffer);

    return vkResult;
}

XR_API VkResult xrInitTextureImageView(XrContext *context, XrTexture *texture)
{
    return xrCreateImageView(context, texture->image->image, texture->format, VK_IMAGE_ASPECT_COLOR_BIT, texture->mipLevels, &(texture->image->imageView));
}

XR_API void xrDestroyTextureImageView(XrContext *context, XrTexture *texture)
{
    if (texture->image && texture->image->imageView)
    {
        vkDestroyImageView(context->device, texture->image->imageView, VK_NULL_HANDLE);
        texture->image->imageView = VK_NULL_HANDLE;
    }
}

XR_API VkResult xrInitTextureSampler(XrContext *context, XrTexture *texture)
{
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.pNext = VK_NULL_HANDLE;
    samplerCreateInfo.flags = 0;
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.anisotropyEnable = VK_TRUE;
    samplerCreateInfo.maxAnisotropy = 16;
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCreateInfo.minLod = 0;
    samplerCreateInfo.maxLod = static_cast<float>(texture->mipLevels);
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

    return vkCreateSampler(context->device, &samplerCreateInfo, VK_NULL_HANDLE, &(texture->sampler));
}

XR_API void xrDestroyTextureSampler(XrContext *context, XrTexture *texture)
{
    if (texture && texture->sampler)
    {
        vkDestroySampler(context->device, texture->sampler, VK_NULL_HANDLE);
        texture->sampler = VK_NULL_HANDLE;
    }
}

XR_API VkResult xrCreateBuffer(XrContext *context, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, XrBuffer *buffer)
{
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.pNext = VK_NULL_HANDLE;
    bufferCreateInfo.flags = 0;
    bufferCreateInfo.size = buffer->memorySize;
    bufferCreateInfo.usage = bufferUsage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = 0;
    bufferCreateInfo.pQueueFamilyIndices = VK_NULL_HANDLE; // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT

    VkResult vkResult = vkCreateBuffer(context->device, &bufferCreateInfo, VK_NULL_HANDLE, &(buffer->buffer));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkMemoryRequirements bufferMemoryRequirements = {};
    vkGetBufferMemoryRequirements(context->device, buffer->buffer, &bufferMemoryRequirements);

    uint32_t memoryIndex = xrFindMemoryTypeIndex(&(context->gpu->memoryProperties), &bufferMemoryRequirements, memoryProperties);

    if (memoryIndex == UINT32_MAX)
    {
        vkResult = VK_ERROR_INITIALIZATION_FAILED;
        return vkResult;
    }

    VkMemoryAllocateInfo memoryAllocationInfo = {};
    memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocationInfo.pNext = VK_NULL_HANDLE;
    memoryAllocationInfo.allocationSize = bufferMemoryRequirements.size;
    memoryAllocationInfo.memoryTypeIndex = memoryIndex;

    vkResult = vkAllocateMemory(context->device, &memoryAllocationInfo, VK_NULL_HANDLE, &(buffer->memory));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = vkBindBufferMemory(context->device, buffer->buffer, buffer->memory, 0);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

XR_API void xrDestroyBuffer(XrContext *context, XrBuffer *buffer)
{
    if (buffer->buffer)
    {
        vkDestroyBuffer(context->device, buffer->buffer, VK_NULL_HANDLE);
        buffer->buffer = VK_NULL_HANDLE;
    }

    if (buffer->memory)
    {
        vkFreeMemory(context->device, buffer->memory, VK_NULL_HANDLE);
        buffer->memory = VK_NULL_HANDLE;
    }
}

XR_API VkResult xrBeginOneTimeCommand(XrContext *context, VkCommandBuffer *commandBuffer)
{
    VkResult vkResult = VK_SUCCESS;

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = VK_NULL_HANDLE;
    commandBufferAllocateInfo.commandPool = context->commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    vkResult = vkAllocateCommandBuffers(context->device, &commandBufferAllocateInfo, commandBuffer);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = VK_NULL_HANDLE;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    commandBufferBeginInfo.pInheritanceInfo = VK_NULL_HANDLE;

    vkResult = vkBeginCommandBuffer(*commandBuffer, &commandBufferBeginInfo);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

XR_API VkResult xrEndOneTimeCommand(XrContext *context, VkCommandBuffer *commandBuffer)
{
    VkResult vkResult = VK_SUCCESS;

    vkResult = vkEndCommandBuffer(*commandBuffer);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = VK_NULL_HANDLE;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = VK_NULL_HANDLE;
    submitInfo.pWaitDstStageMask = VK_NULL_HANDLE;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = VK_NULL_HANDLE;

    vkResult = vkQueueSubmit(context->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = vkQueueWaitIdle(context->graphicsQueue);
    vkFreeCommandBuffers(context->device, context->commandPool, 1, commandBuffer);

    return vkResult;
}

VkResult xrTransitionImageLayout(
    XrContext *context,
    VkImage *image,
    VkFormat format,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    uint32_t mipLevels
)
{
    VkResult vkResult = VK_SUCCESS;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkResult = xrBeginOneTimeCommand(context, &commandBuffer);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = VK_NULL_HANDLE;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = *image;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = mipLevels;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStageMask;
    VkPipelineStageFlags destinationStageMask;

    if (oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        vkResult = VK_ERROR_NOT_PERMITTED;
        return vkResult;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStageMask, destinationStageMask, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &imageMemoryBarrier);

    vkResult = xrEndOneTimeCommand(context, &commandBuffer);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

XR_API VkResult xrCopyBuffer(XrContext *context, VkBuffer sourceBuffer, VkBuffer targetBuffer, VkDeviceSize size)
{
    VkResult vkResult = VK_SUCCESS;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkResult = xrBeginOneTimeCommand(context, &commandBuffer);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    vkCmdCopyBuffer(commandBuffer, sourceBuffer, targetBuffer, 1, &copyRegion);

    vkResult = xrEndOneTimeCommand(context, &commandBuffer);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

XR_API VkResult xrCopyBufferToImage(XrContext *context, VkBuffer buffer, VkImage image, VkExtent2D *extent)
{
    VkResult vkResult = VK_SUCCESS;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkResult = xrBeginOneTimeCommand(context, &commandBuffer);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;
    region.imageExtent.width = extent->width;
    region.imageExtent.height = extent->height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    vkResult = xrEndOneTimeCommand(context, &commandBuffer);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

XR_API VkResult xrInitVertexBuffer(XrContext *context, XrModel *model)
{
    VkResult vkResult = VK_SUCCESS;

    model->vertexBuffer = (XrBuffer *)malloc(sizeof(XrBuffer));
    memset((void *)model->vertexBuffer, 0, sizeof(XrBuffer));
    model->vertexBuffer->memorySize = sizeof(XrVertex) * model->verticesCount;

    VkBufferUsageFlags stagingBufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags stagingMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    XrBuffer stagingBuffer = {};
    stagingBuffer.memorySize = model->vertexBuffer->memorySize;

    vkResult = xrCreateBuffer(context, stagingBufferUsage, stagingMemoryProperties, &stagingBuffer);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    void *stagingBufferData = VK_NULL_HANDLE;
    vkResult = vkMapMemory(context->device, stagingBuffer.memory, 0, stagingBuffer.memorySize, 0, &stagingBufferData);

    if (XR_IS_ERROR(vkResult))
    {
        xrDestroyBuffer(context, &stagingBuffer);
        return vkResult;
    }

    memcpy(stagingBufferData, model->vertices, (size_t)stagingBuffer.memorySize);
    vkUnmapMemory(context->device, stagingBuffer.memory);

    VkBufferUsageFlags vertexBufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VkMemoryPropertyFlags vertexMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    vkResult = xrCreateBuffer(context, vertexBufferUsage, vertexMemoryProperties, model->vertexBuffer);

    if (XR_IS_ERROR(vkResult))
    {
        xrDestroyBuffer(context, &stagingBuffer);
        return vkResult;
    }

    vkResult = xrCopyBuffer(context, stagingBuffer.buffer, model->vertexBuffer->buffer, model->vertexBuffer->memorySize);

    if (XR_IS_ERROR(vkResult))
    {
        xrDestroyBuffer(context, &stagingBuffer);
        return vkResult;
    }

    xrDestroyBuffer(context, &stagingBuffer);
    return vkResult;
}

XR_API void xrDestroyVertexBuffer(XrContext *context, XrModel *model)
{
    if (model->vertexBuffer)
    {
        xrDestroyBuffer(context, model->vertexBuffer);
    }

    XR_FREE(model->vertexBuffer);
}

XR_API VkResult xrInitIndexBuffer(XrContext *context, XrModel *model)
{
    VkResult vkResult = VK_SUCCESS;

    model->indexBuffer = (XrBuffer *)malloc(sizeof(XrBuffer));
    memset((void *)model->indexBuffer, 0, sizeof(XrBuffer));
    model->indexBuffer->memorySize = sizeof(uint32_t) * model->vertexIndicesCount;

    VkBufferUsageFlags stagingBufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags stagingMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    XrBuffer stagingBuffer = {};
    stagingBuffer.memorySize = model->indexBuffer->memorySize;

    vkResult = xrCreateBuffer(context, stagingBufferUsage, stagingMemoryProperties, &stagingBuffer);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    void *stagingBufferData = VK_NULL_HANDLE;
    vkResult = vkMapMemory(context->device, stagingBuffer.memory, 0, stagingBuffer.memorySize, 0, &stagingBufferData);
    if (XR_IS_ERROR(vkResult))
    {
        xrDestroyBuffer(context, &stagingBuffer);
        return vkResult;
    }

    memcpy(stagingBufferData, model->vertexIndices, (size_t)stagingBuffer.memorySize);
    vkUnmapMemory(context->device, stagingBuffer.memory);

    VkBufferUsageFlags indexBufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    VkMemoryPropertyFlags indexMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    vkResult = xrCreateBuffer(context, indexBufferUsage, indexMemoryProperties, model->indexBuffer);

    if (XR_IS_ERROR(vkResult))
    {
        xrDestroyBuffer(context, &stagingBuffer);
        return vkResult;
    }

    vkResult = xrCopyBuffer(context, stagingBuffer.buffer, model->indexBuffer->buffer, model->indexBuffer->memorySize);
    xrDestroyBuffer(context, &stagingBuffer);

    return vkResult;
}

XR_API void xrDestroyIndexBuffer(XrContext *context, XrModel *model)
{
    if (model->indexBuffer)
    {
        xrDestroyBuffer(context, model->indexBuffer);
    }

    XR_FREE(model->indexBuffer);
}

XR_API VkResult xrInitUniformBuffers(XrContext *context, XrModel *model)
{
    VkResult vkResult = VK_SUCCESS;

    VkDeviceSize memorySize = sizeof(XrUniformBuffer);
    VkBufferUsageFlags uniformBufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    VkMemoryPropertyFlags uniformMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    model->uniformBuffersCount = context->swapchainImageCount;
    model->uniformBuffers = (XrBuffer *)malloc(sizeof(XrBuffer) * model->uniformBuffersCount);
    memset((void *)model->uniformBuffers, 0, sizeof(XrBuffer) * model->uniformBuffersCount);

    for (size_t counter = 0; counter < model->uniformBuffersCount; ++counter)
    {
        model->uniformBuffers[counter].memorySize = memorySize;

        vkResult = xrCreateBuffer(context, uniformBufferUsage, uniformMemoryProperties, &(model->uniformBuffers[counter]));

        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }
    }

    return vkResult;
}

XR_API void xrDestroyUniformBuffers(XrContext *context, XrModel *model)
{
    if (model->uniformBuffers)
    {
        for (size_t counter = 0; counter < model->uniformBuffersCount; ++counter)
        {
            xrDestroyBuffer(context, &model->uniformBuffers[counter]);
        }
    }

    XR_FREE(model->uniformBuffers);
}

XR_API VkResult xrInitDescriptorPool(XrContext *context, uint32_t count)
{
    VkDescriptorPoolSize uboPoolSize = {};
    uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboPoolSize.descriptorCount = context->swapchainImageCount * count;

    VkDescriptorPoolSize samplerPoolSize = {};
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = context->swapchainImageCount * count;

    VkDescriptorPoolSize poolSizes[2] = {uboPoolSize, samplerPoolSize};

    VkDescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.pNext = VK_NULL_HANDLE;
    poolCreateInfo.flags = 0;
    // If you want to explicitly destroy the descriptorSet, then set this bit
    // else you will get runtime error while destroying the descriptorSet.
    // We are not going to used this for now.
    // poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolCreateInfo.maxSets = context->swapchainImageCount * count;
    poolCreateInfo.poolSizeCount = _ARRAYSIZE(poolSizes);
    poolCreateInfo.pPoolSizes = poolSizes;

    return vkCreateDescriptorPool(context->device, &poolCreateInfo, VK_NULL_HANDLE, &(context->descriptorPool));
}

XR_API void xrDestroyDescriptorPool(XrContext *context)
{
    if (context->descriptorPool)
    {
        vkDestroyDescriptorPool(context->device, context->descriptorPool, VK_NULL_HANDLE);
        context->descriptorPool = VK_NULL_HANDLE;
    }
}

XR_API VkResult xrInitDescriptorSets(XrContext *context, XrModel **models, uint32_t moduleCount)
{
    VkResult vkResult = VK_SUCCESS;
    VkDescriptorSetLayout *descriptorSetLayouts = (VkDescriptorSetLayout *)malloc(sizeof(VkDescriptorSetLayout) * context->swapchainImageCount);
    memset((void *)descriptorSetLayouts, 0, sizeof(VkDescriptorSetLayout) * context->swapchainImageCount);

    for (size_t index = 0; index < context->swapchainImageCount; ++index)
    {
        memcpy(&descriptorSetLayouts[index], &context->descriptorSetLayout, sizeof(VkDescriptorSetLayout));
    }

    for (size_t index = 0; index < moduleCount; ++index)
    {
        XrModel *model = models[index];
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.pNext = VK_NULL_HANDLE;
        descriptorSetAllocateInfo.descriptorPool = context->descriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = context->swapchainImageCount;
        descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts;

        model->descriptorSetsCount = context->swapchainImageCount;
        model->descriptorSets = (VkDescriptorSet *)malloc(sizeof(VkDescriptorSet) * model->descriptorSetsCount);
        memset((void *)model->descriptorSets, 0, sizeof(VkDescriptorSet) * model->descriptorSetsCount);

        vkResult = vkAllocateDescriptorSets(context->device, &descriptorSetAllocateInfo, model->descriptorSets);
        if (XR_IS_ERROR(vkResult))
        {
            XR_FREE(descriptorSetLayouts);
            return vkResult;
        }

        for (size_t counter = 0; counter < model->uniformBuffersCount; ++counter)
        {
            VkDescriptorBufferInfo descriptorBufferInfo = {};
            descriptorBufferInfo.buffer = model->uniformBuffers[counter].buffer;
            descriptorBufferInfo.offset = 0;
            descriptorBufferInfo.range = sizeof(XrUniformBuffer);

            VkDescriptorImageInfo descriptorImageInfo = {};
            descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorImageInfo.imageView = model->texture->image->imageView;
            descriptorImageInfo.sampler = model->texture->sampler;

            VkWriteDescriptorSet uniformBudderDescriptorWrite = {};
            uniformBudderDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            uniformBudderDescriptorWrite.pNext = VK_NULL_HANDLE;
            uniformBudderDescriptorWrite.dstSet = model->descriptorSets[counter];
            uniformBudderDescriptorWrite.dstBinding = 0;
            uniformBudderDescriptorWrite.dstArrayElement = 0;
            uniformBudderDescriptorWrite.descriptorCount = 1;
            uniformBudderDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uniformBudderDescriptorWrite.pImageInfo = VK_NULL_HANDLE;
            uniformBudderDescriptorWrite.pBufferInfo = &descriptorBufferInfo;
            uniformBudderDescriptorWrite.pTexelBufferView = VK_NULL_HANDLE;

            VkWriteDescriptorSet textureImageDescriptorWrite = {};
            textureImageDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            textureImageDescriptorWrite.pNext = VK_NULL_HANDLE;
            textureImageDescriptorWrite.dstSet = model->descriptorSets[counter];
            textureImageDescriptorWrite.dstBinding = 1;
            textureImageDescriptorWrite.dstArrayElement = 0;
            textureImageDescriptorWrite.descriptorCount = 1;
            textureImageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            textureImageDescriptorWrite.pImageInfo = &descriptorImageInfo;
            textureImageDescriptorWrite.pBufferInfo = VK_NULL_HANDLE;
            textureImageDescriptorWrite.pTexelBufferView = VK_NULL_HANDLE;

            VkWriteDescriptorSet descriptorWrites[2] = {uniformBudderDescriptorWrite, textureImageDescriptorWrite};
            vkUpdateDescriptorSets(context->device, _ARRAYSIZE(descriptorWrites), descriptorWrites, 0, VK_NULL_HANDLE);
        }
    }

    XR_FREE(descriptorSetLayouts);

    return vkResult;
}

XR_API void xrDestroyDescriptorSets(XrContext *context, XrModel **models, uint32_t moduleCount)
{
    for (size_t index = 0; index < moduleCount; ++index)
    {
        XrModel *model = models[index];

        // If you want to explicitly destroy the descriptorSet, then set
        // poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
        // bit in VkDescriptorPoolCreateInfo else you will get runtime error
        // while destroying the descriptorSet.
        // We are not going to used this for now.
        // vkFreeDescriptorSets(
        //     context->device,
        //     context->descriptorPool,
        //     model->descriptorSets.size(),
        //     model->descriptorSets.data()
        // );

        XR_FREE(model->descriptorSets);
    }
}

XR_API VkResult xrInitCommandBuffers(XrContext *context, XrModel **models, uint32_t moduleCount)
{
    VkResult vkResult = VK_SUCCESS;

    context->commandBuffers = (VkCommandBuffer *)malloc(sizeof(VkCommandBuffer) * context->swapchainImageCount);
    memset((void *)context->commandBuffers, 0, sizeof(VkCommandBuffer) * context->swapchainImageCount);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = VK_NULL_HANDLE;
    commandBufferAllocateInfo.commandPool = context->commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = context->swapchainImageCount;

    vkResult = vkAllocateCommandBuffers(context->device, &commandBufferAllocateInfo, context->commandBuffers);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    for (uint32_t counter = 0; counter < context->swapchainImageCount; ++counter)
    {
        VkCommandBufferBeginInfo commandBufferBeginInfo = {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.pNext = VK_NULL_HANDLE;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        commandBufferBeginInfo.pInheritanceInfo = VK_NULL_HANDLE;

        vkResult = vkBeginCommandBuffer(context->commandBuffers[counter], &commandBufferBeginInfo);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        VkRect2D renderArea = {};
        renderArea.offset.x = 0;
        renderArea.offset.y = 0;
        renderArea.extent.width = context->surfaceExtent.width;
        renderArea.extent.height = context->surfaceExtent.height;

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = VK_NULL_HANDLE;
        renderPassBeginInfo.renderPass = context->renderPass;
        renderPassBeginInfo.framebuffer = context->framebuffers[counter];
        renderPassBeginInfo.renderArea = renderArea;
        renderPassBeginInfo.clearValueCount = context->clearValueCount;
        renderPassBeginInfo.pClearValues = context->clearValue;

        vkCmdBeginRenderPass(context->commandBuffers[counter], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(context->commandBuffers[counter], VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline);

        VkDeviceSize offset = {0};

        for (size_t index = 0; index < moduleCount; ++index)
        {
            XrModel *model = models[index];
            vkCmdBindVertexBuffers(context->commandBuffers[counter], 0, 1, &(model->vertexBuffer->buffer), &offset);
            vkCmdBindIndexBuffer(context->commandBuffers[counter], model->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(
                context->commandBuffers[counter],
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                context->pipelineLayout,
                0,
                1,
                &(model->descriptorSets[counter]),
                0,
                VK_NULL_HANDLE
            );

            vkCmdDrawIndexed(context->commandBuffers[counter], model->vertexIndicesCount, 1, 0, 0, 0);
        }

        vkCmdEndRenderPass(context->commandBuffers[counter]);

        vkResult = vkEndCommandBuffer(context->commandBuffers[counter]);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }
    }

    return vkResult;
}

XR_API void xrDestroyCommandBuffers(XrContext *context)
{
    if (context->commandBuffers)
    {
        vkFreeCommandBuffers(context->device, context->commandPool, context->swapchainImageCount, context->commandBuffers);
    }

    XR_FREE(context->commandBuffers);
}

XR_API VkResult xrInitSynchronizations(XrContext *context)
{
    VkResult vkResult = VK_SUCCESS;

    context->imageAvailableSemaphores = (VkSemaphore *)malloc(sizeof(VkSemaphore) * context->swapchainImageCount);
    memset((void *)context->imageAvailableSemaphores, 0, sizeof(VkSemaphore) * context->swapchainImageCount);

    context->renderFinishedSemaphores = (VkSemaphore *)malloc(sizeof(VkSemaphore) * context->swapchainImageCount);
    memset((void *)context->renderFinishedSemaphores, 0, sizeof(VkSemaphore) * context->swapchainImageCount);

    context->inFlightFences = (VkFence *)malloc(sizeof(VkFence) * context->swapchainImageCount);
    memset((void *)context->inFlightFences, 0, sizeof(VkFence) * context->swapchainImageCount);

    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = VK_NULL_HANDLE;
    semaphoreCreateInfo.flags = 0;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = VK_NULL_HANDLE;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t counter = 0; counter < context->swapchainImageCount; ++counter)
    {
        vkResult = vkCreateSemaphore(context->device, &semaphoreCreateInfo, VK_NULL_HANDLE, &context->imageAvailableSemaphores[counter]);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        vkResult = vkCreateSemaphore(context->device, &semaphoreCreateInfo, VK_NULL_HANDLE, &context->renderFinishedSemaphores[counter]);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        vkResult = vkCreateFence(context->device, &fenceCreateInfo, VK_NULL_HANDLE, &(context->inFlightFences[counter]));
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }
    }

    return vkResult;
}

XR_API void xrDestroySynchronizations(XrContext *context)
{
    for (size_t counter = 0; counter < context->swapchainImageCount; ++counter)
    {
        if (context->imageAvailableSemaphores && context->imageAvailableSemaphores[counter])
        {
            vkDestroySemaphore(context->device, context->imageAvailableSemaphores[counter], VK_NULL_HANDLE);
            context->imageAvailableSemaphores[counter] = VK_NULL_HANDLE;
        }

        if (context->renderFinishedSemaphores && context->renderFinishedSemaphores[counter])
        {
            vkDestroySemaphore(context->device, context->renderFinishedSemaphores[counter], VK_NULL_HANDLE);
            context->renderFinishedSemaphores[counter] = VK_NULL_HANDLE;
        }

        if (context->inFlightFences && context->inFlightFences[counter])
        {
            vkDestroyFence(context->device, context->inFlightFences[counter], VK_NULL_HANDLE);
            context->inFlightFences[counter] = VK_NULL_HANDLE;
        }
    }

    XR_FREE(context->imageAvailableSemaphores);
    XR_FREE(context->renderFinishedSemaphores);
    XR_FREE(context->inFlightFences);
}

XR_API VkResult xrRecreateSwapChain(XrContext *context, XrModel **models, uint32_t moduleCount)
{
    VkResult vkResult = VK_SUCCESS;

    XR_LOG_INFO(context->logger, "---------- Recreate SwapChain --------");
    xrCleanupSwapChain(context, models, moduleCount);

    vkResult = xrInitSwapchain(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitSwapchainImageViews(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitDepthStencilImage(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitMSAAColorImage(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitFrameBuffers(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitRenderPass(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitDescriptorSetLayout(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitDescriptorPool(context, moduleCount);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitDescriptorSets(context, models, moduleCount);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitGraphicsPiplineCache(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitGraphicsPipline(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitSynchronizations(context);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = xrInitCommandBuffers(context, models, moduleCount);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    XR_LOG_INFO(context->logger, "---------- Recreate SwapChain done --------");

    return vkResult;
}

XR_API void xrCleanupSwapChain(XrContext *context, XrModel **models, uint32_t moduleCount)
{
    xrWaitForIdle(context);
    xrDestroyCommandBuffers(context);
    xrDestroySynchronizations(context);
    xrDestroyGraphicsPipline(context);
    xrDestroyGraphicsPiplineCache(context);
    xrDestroyDescriptorSets(context, models, moduleCount);
    xrDestroyDescriptorPool(context);
    xrDestroyDescriptorSetLayout(context);

    xrDestroyRenderPass(context);
    xrDestroyFrameBuffers(context);
    xrDestroyMSAAColorImage(context);
    xrDestroyDepthStencilImage(context);
    xrDestroySwapchainImageViews(context);
    xrDestroySwapchain(context);
}

XR_API VkResult xrRender(XrContext *context, XrModel **models, uint32_t moduleCount)
{
    VkResult vkResult = VK_SUCCESS;

    // Update the current frame count at start as we might return in between and fail to update the counter
    context->currentFrame = (context->currentFrame + 1) % context->swapchainImageCount;

    vkResult = vkWaitForFences(context->device, 1, &(context->inFlightFences[context->currentFrame]), VK_TRUE, UINT64_MAX);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    uint32_t activeSwapchainImageId = UINT32_MAX;

    vkResult = vkAcquireNextImageKHR(
        context->device, context->swapchain, UINT64_MAX, context->imageAvailableSemaphores[context->currentFrame], VK_NULL_HANDLE, &activeSwapchainImageId
    );

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = vkResetFences(context->device, 1, &(context->inFlightFences[context->currentFrame]));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    // Update the uniform buffer for current image.
    xrUpdateUniformBuffer(context, models, moduleCount, activeSwapchainImageId);

    VkSemaphore waitSemaphores[] = {context->imageAvailableSemaphores[context->currentFrame]};
    VkSemaphore signalSemaphores[] = {context->renderFinishedSemaphores[context->currentFrame]};
    VkPipelineStageFlags waitPipelineStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = VK_NULL_HANDLE;
    submitInfo.waitSemaphoreCount = _ARRAYSIZE(waitSemaphores);
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitPipelineStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &(context->commandBuffers[activeSwapchainImageId]);
    submitInfo.signalSemaphoreCount = _ARRAYSIZE(signalSemaphores);
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResult = vkQueueSubmit(context->graphicsQueue, 1, &submitInfo, context->inFlightFences[context->currentFrame]);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkSwapchainKHR swapchains[] = {context->swapchain};

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = VK_NULL_HANDLE;
    presentInfo.waitSemaphoreCount = _ARRAYSIZE(signalSemaphores);
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = _ARRAYSIZE(swapchains);
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &activeSwapchainImageId;
    presentInfo.pResults = VK_NULL_HANDLE;

    vkResult = vkQueuePresentKHR(context->presentQueue, &presentInfo);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

void xrUpdateUniformBuffer(XrContext *context, XrModel **models, uint32_t moduleCount, uint32_t imageIndex)
{
    VkResult vkResult = VK_SUCCESS;
    XrUniformBuffer *ubo = (XrUniformBuffer *)malloc(sizeof(XrUniformBuffer));
    memset((void *)ubo, 0, sizeof(XrUniformBuffer));

    for (size_t index = 0; index < moduleCount; ++index)
    {
        XrModel *model = models[index];
        memset((void *)ubo, 0, sizeof(XrUniformBuffer));

        void *data = VK_NULL_HANDLE;
        vkResult = vkMapMemory(context->device, model->uniformBuffers[imageIndex].memory, 0, sizeof(XrUniformBuffer), 0, &data);

        if (XR_IS_ERROR(vkResult))
        {
            XR_LOG_ERROR(context->logger, "Failed to update uniform buffer for model (%d)", vkResult);
            continue;
        }

        if (model->draw)
        {
            model->draw(ubo);
        }

        memcpy(data, ubo, sizeof(XrUniformBuffer));
        vkUnmapMemory(context->device, model->uniformBuffers[imageIndex].memory);
    }

    XR_FREE(ubo);
}

// Debug methods

XR_API void xrPrintInstanceLayerProperties(XrContext *context, VkLayerProperties *properties, uint32_t propertiesCount)
{
    XR_LOG_INFO(context->logger, "---------- Instance Layer Properties [%d] ----------", propertiesCount);

    for (uint32_t index = 0; index < propertiesCount; ++index)
    {
        XR_LOG_INFO(context->logger, "---------- Instance Layer Property [%d/%d] ----------", index, propertiesCount);
        VkLayerProperties *nextProperty = &properties[index];
        XR_LOG_INFO(context->logger, "Layer Name             : %s", nextProperty->layerName);
        XR_LOG_INFO(context->logger, "Description            : %s", nextProperty->description);
        XR_LOG_INFO(context->logger, "Spec Version           : %d", nextProperty->specVersion);
        XR_LOG_INFO(context->logger, "Implementation Version : %d", nextProperty->implementationVersion);
        XR_LOG_INFO(context->logger, "------------------------------------------------------------");
    }

    XR_LOG_INFO(context->logger, "---------- Instance Layer Properties End ----------");
}

XR_API void xrPrintDeviceLayerProperties(XrContext *context, VkLayerProperties *properties, uint32_t propertiesCount)
{
    XR_LOG_INFO(context->logger, "---------- Device Layer Property [%d] ----------", propertiesCount);

    for (uint32_t index = 0; index < propertiesCount; ++index)
    {
        XR_LOG_INFO(context->logger, "---------- Device Layer Properties [%d/%d] ----------", index, propertiesCount);
        VkLayerProperties *nextProperty = &properties[index];
        XR_LOG_INFO(context->logger, "Layer Name             : %s", nextProperty->layerName);
        XR_LOG_INFO(context->logger, "Description            : %s", nextProperty->description);
        XR_LOG_INFO(context->logger, "Spec Version           : %d", nextProperty->specVersion);
        XR_LOG_INFO(context->logger, "Implementation Version : %d", nextProperty->implementationVersion);
        XR_LOG_INFO(context->logger, "------------------------------------------------------------");
    }

    XR_LOG_INFO(context->logger, "---------- Device Layer Properties End ----------");
}

XR_API void xrPrintSurfaceFormatsDetails(XrContext *context, VkSurfaceFormatKHR *surfaceFormats, uint32_t surfaceFormatsCount)
{
    XR_LOG_INFO(context->logger, "---------- Surface Formats [%d] ----------", surfaceFormatsCount);

    for (uint32_t index = 0; index < surfaceFormatsCount; ++index)
    {
        XR_LOG_INFO(context->logger, "---------- Surface Format [%d/%d] ----------", index, surfaceFormatsCount);
        VkSurfaceFormatKHR *nextSurfaceFormat = &surfaceFormats[index];
        XR_LOG_INFO(context->logger, "format        : %d", nextSurfaceFormat->format);
        XR_LOG_INFO(context->logger, "colorSpace    : %d", nextSurfaceFormat->colorSpace);
        XR_LOG_INFO(context->logger, "------------------------------------------------------------");
    }

    XR_LOG_INFO(context->logger, "---------- Surface Formats End ----------");
}

XR_API void xrPrintSwapChainImageCount(XrContext *context, uint32_t minImageCount, uint32_t maxImageCount, uint32_t currentImageCount)
{
    XR_LOG_INFO(context->logger, "---------- Swapchain Image Count ----------");
    XR_LOG_INFO(context->logger, "Min\t: %d", minImageCount);
    XR_LOG_INFO(context->logger, "Max\t: %d", maxImageCount);
    XR_LOG_INFO(context->logger, "Current\t: %d", currentImageCount);
    XR_LOG_INFO(context->logger, "---------- Swapchain Image Count End ----------");
}
