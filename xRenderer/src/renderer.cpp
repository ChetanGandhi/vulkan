#include "platform.h"
#include "renderer.h"
#include "utils.h"
#include "logger.h"
#include "debugger.h"
#include "instance.h"

XR_API VkResult xrInitInstance(XrContext *context, VkApplicationInfo *applicationInfo)
{
    xrSetupLayersAndExtensions(context);

    VkDebugUtilsMessengerCreateInfoEXT vkDebugUtilsMessengerCreateInfo;
    memset((void *)&vkDebugUtilsMessengerCreateInfo, 0, sizeof(VkDebugUtilsMessengerCreateInfoEXT));
    xrFillDebuggerCreateInfo(context, &vkDebugUtilsMessengerCreateInfo);

    VkResult vkResult = xrIsValidationLayerSupport(context, "VK_LAYER_KHRONOS_validation");

    if (XR_CHECK_RESULT(vkResult, VK_SUCCESS))
    {
        context->instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
    }

    vkResult = xrCreateVulkanInstance(context, applicationInfo, &(context->instanceLayers), &(context->instanceExtensions), &vkDebugUtilsMessengerCreateInfo);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    if (XR_IS_ERROR(xrCreateDebugger(context, &vkDebugUtilsMessengerCreateInfo)))
    {
        XR_LOG_ERROR(context->logger, xrErrorName(vkResult), "Failed to create debugger");
    }

    return vkResult;
}

XR_API VkResult xrDestroyInstance(XrContext *context)
{
    xrDestroyDebugger(context);
    xrDestroyVulkanInstance(context);

    return VK_SUCCESS;
}

void xrSetupLayersAndExtensions(XrContext *context)
{
    context->instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    context->instanceExtensions.push_back(PLATFORM_SURFACE_EXTENSION_NAME);
    context->instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    context->deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

XR_API void xrWaitForIdle(XrContext *context)
{
    vkDeviceWaitIdle(context->device);
}

XR_API void xrListAllPhysicalDevices(XrContext *context, std::vector<XrGpuDetails> *gpuDetailsList)
{
    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(context->instance, &gpuCount, VK_NULL_HANDLE);

    if (gpuCount == 0)
    {
        return;
    }

    std::vector<VkPhysicalDevice> deviceList(gpuCount);
    vkEnumeratePhysicalDevices(context->instance, &gpuCount, deviceList.data());

    for (uint32_t counter = 0; counter < gpuCount; ++counter)
    {
        VkPhysicalDevice nextGpu = deviceList[counter];
        VkPhysicalDeviceProperties nextGpuProperties = {};
        VkPhysicalDeviceMemoryProperties nextGpuMemoryProperties = {};

        vkGetPhysicalDeviceProperties(nextGpu, &nextGpuProperties);
        vkGetPhysicalDeviceMemoryProperties(nextGpu, &nextGpuMemoryProperties);

        XrGpuDetails nextPhysicalDevice = {};
        nextPhysicalDevice.gpu = nextGpu;
        nextPhysicalDevice.properties = nextGpuProperties;
        nextPhysicalDevice.memoryProperties = nextGpuMemoryProperties;
        gpuDetailsList->push_back(nextPhysicalDevice);
    }
}

XR_API uint32_t xrFindMemoryTypeIndex(
    const VkPhysicalDeviceMemoryProperties *gpuMemoryProperties,
    const VkMemoryRequirements *imageMemoryRequirements,
    const VkMemoryPropertyFlags requiredMemoryProperties
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

void xrRankDevice(XrContext *context, XrGpuDetails *gpuDetails)
{
    // Higher the rank, more suitable the device
    uint32_t rank = 0;
    bool extensionSupported = xrCheckDeviceExtensionSupport(context, gpuDetails->gpu);
    bool swapchainSupported = false;

    if (extensionSupported)
    {
        XrSwapchainSupportDetails details = {};
        xrQuerySwapchainSupportDetails(context, gpuDetails->gpu, &details);
        swapchainSupported = !details.surfaceFormats.empty() && !details.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures = {};
    vkGetPhysicalDeviceFeatures(gpuDetails->gpu, &supportedFeatures);

    // Increase the rank for each check
    if (gpuDetails->graphicsFamilyIndex != UINT32_MAX)
    {
        rank++;
    }

    if (gpuDetails->hasSeparatePresentQueue && gpuDetails->presentFamilyIndex != UINT32_MAX)
    {
        rank++;
    }

    if (extensionSupported)
    {
        rank++;
    }

    if (swapchainSupported)
    {
        rank++;
    }

    if (supportedFeatures.samplerAnisotropy)
    {
        rank++;
    }

    if (supportedFeatures.geometryShader)
    {
        rank++;
    }

    if (supportedFeatures.tessellationShader)
    {
        rank++;
    }

    if (gpuDetails->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        rank += 2;
    }
    else if (gpuDetails->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        rank++;
    }

    gpuDetails->rank = rank;
}

void xrFindSuitableDeviceQueues(XrContext *context, XrGpuDetails *gpuDetails)
{
    uint32_t familyCount = 0;
    uint32_t graphicsFamilyIndex = UINT32_MAX;
    uint32_t presentFamilyIndex = UINT32_MAX;

    vkGetPhysicalDeviceQueueFamilyProperties(gpuDetails->gpu, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> familyPropertiesList(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(gpuDetails->gpu, &familyCount, familyPropertiesList.data());

    for (uint32_t queueCounter = 0; queueCounter < familyCount; ++queueCounter)
    {
        const VkQueueFamilyProperties nextFamilyProperties = familyPropertiesList[queueCounter];

        if (nextFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsFamilyIndex = queueCounter;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(gpuDetails->gpu, queueCounter, context->surface, &presentSupport);

        if (presentSupport == VK_TRUE)
        {
            graphicsFamilyIndex = queueCounter;
            presentFamilyIndex = queueCounter;
            break;
        }
    }

    if (presentFamilyIndex == UINT32_MAX)
    {
        for (uint32_t queueCounter = 0; queueCounter < familyCount; ++queueCounter)
        {
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(gpuDetails->gpu, queueCounter, context->surface, &presentSupport);

            if (presentSupport == VK_TRUE)
            {
                presentFamilyIndex = queueCounter;
                break;
            }
        }
    }

    gpuDetails->graphicsFamilyIndex = graphicsFamilyIndex;
    gpuDetails->presentFamilyIndex = presentFamilyIndex;
    gpuDetails->hasSeparatePresentQueue = (presentFamilyIndex != graphicsFamilyIndex);
}

void xrFindMaxMSAASampleCount(XrGpuDetails *gpuDetails)
{
    VkSampleCountFlags sampleCountFlags =
        gpuDetails->properties.limits.framebufferColorSampleCounts & gpuDetails->properties.limits.framebufferDepthSampleCounts;

    if (sampleCountFlags & VK_SAMPLE_COUNT_64_BIT)
    {
        gpuDetails->msaaSamples = VK_SAMPLE_COUNT_64_BIT;
    }
    else if (sampleCountFlags & VK_SAMPLE_COUNT_32_BIT)
    {
        gpuDetails->msaaSamples = VK_SAMPLE_COUNT_32_BIT;
    }
    else if (sampleCountFlags & VK_SAMPLE_COUNT_16_BIT)
    {
        gpuDetails->msaaSamples = VK_SAMPLE_COUNT_16_BIT;
    }
    else if (sampleCountFlags & VK_SAMPLE_COUNT_8_BIT)
    {
        gpuDetails->msaaSamples = VK_SAMPLE_COUNT_8_BIT;
    }
    else if (sampleCountFlags & VK_SAMPLE_COUNT_4_BIT)
    {
        gpuDetails->msaaSamples = VK_SAMPLE_COUNT_4_BIT;
    }
    else if (sampleCountFlags & VK_SAMPLE_COUNT_2_BIT)
    {
        gpuDetails->msaaSamples = VK_SAMPLE_COUNT_2_BIT;
    }
    else
    {
        gpuDetails->msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    }
}

bool xrCheckDeviceExtensionSupport(XrContext *context, VkPhysicalDevice gpu)
{
    uint32_t availableDeviceExtensionsCount = 0;

    VkResult vkResult = vkEnumerateDeviceExtensionProperties(gpu, nullptr, &availableDeviceExtensionsCount, nullptr);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    if (availableDeviceExtensionsCount == 0)
    {
        return false;
    }

    std::vector<VkExtensionProperties> availableDeviceExtensions(availableDeviceExtensionsCount);
    vkResult = vkEnumerateDeviceExtensionProperties(gpu, nullptr, &availableDeviceExtensionsCount, availableDeviceExtensions.data());

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    std::set<std::string> requiredExtensions(context->deviceExtensions.begin(), context->deviceExtensions.end());

    for (const VkExtensionProperties &nextExtensionProperties : availableDeviceExtensions)
    {
        requiredExtensions.erase(nextExtensionProperties.extensionName);
    }

    return requiredExtensions.empty();
}

XR_API VkResult xrInitDevice(XrContext *context)
{
    std::vector<XrGpuDetails> gpuDetailsList(0);
    xrListAllPhysicalDevices(context, &gpuDetailsList);

    uint32_t gpuCount = static_cast<uint32_t>(gpuDetailsList.size());
    uint32_t lastRank = 0;
    int32_t selectedGpuIndex = -1;

    XR_LOG_INFO(context->logger, "---------- Total GPU Found [%d]----------", gpuCount);

    for (uint32_t counter = 0; counter < gpuCount; ++counter)
    {
        XrGpuDetails *nextGpuDetails = &gpuDetailsList[counter];
        xrFindSuitableDeviceQueues(context, nextGpuDetails);
        xrFindMaxMSAASampleCount(nextGpuDetails);
        xrRankDevice(context, nextGpuDetails);
        xrPrintGpuProperties(context, nextGpuDetails, counter + 1, gpuCount);

        if (lastRank < nextGpuDetails->rank)
        {
            lastRank = nextGpuDetails->rank;
            selectedGpuIndex = counter;
        }
    }

    if (selectedGpuIndex > -1)
    {
        memcpy((void *)&(context->gpuDetails), &gpuDetailsList[selectedGpuIndex], sizeof(XrGpuDetails));
    }
    else
    {
        XR_LOG_INFO(context->logger, "Vulkan Error: Unable to find suitable graphics device.");
        assert(0 && "Vulkan Error: Unable to find suitable graphics device.");
        std::exit(EXIT_FAILURE);
    }

    XR_LOG_INFO(context->logger, "---------- Selected GPU Properties ----------");

    xrPrintGpuProperties(context, &(context->gpuDetails), selectedGpuIndex + 1, gpuCount);

    XR_LOG_INFO(context->logger, "---------- Selected GPU Properties End ----------");

    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, VK_NULL_HANDLE);
        std::vector<VkLayerProperties> layerPropertiesList(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layerPropertiesList.data());
        xrPrintInstanceLayerProperties(context, layerPropertiesList);
    }

    {
        uint32_t layerCount = 0;
        vkEnumerateDeviceLayerProperties(context->gpuDetails.gpu, &layerCount, VK_NULL_HANDLE);
        std::vector<VkLayerProperties> layerPropertiesList(layerCount);
        vkEnumerateDeviceLayerProperties(context->gpuDetails.gpu, &layerCount, layerPropertiesList.data());
        xrPrintDeviceLayerProperties(context, layerPropertiesList);
    }

    return VK_SUCCESS;
}

XR_API VkResult xrInitLogicalDevice(XrContext *context)
{
    std::vector<float> queuePriorities = {0.0f};
    std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos(0);

    VkDeviceQueueCreateInfo deviceGraphicQueueCreateInfo = {};
    deviceGraphicQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceGraphicQueueCreateInfo.pNext = nullptr;
    deviceGraphicQueueCreateInfo.flags = 0;
    deviceGraphicQueueCreateInfo.queueFamilyIndex = context->gpuDetails.graphicsFamilyIndex;
    deviceGraphicQueueCreateInfo.queueCount = 1;
    deviceGraphicQueueCreateInfo.pQueuePriorities = queuePriorities.data();

    deviceQueueCreateInfos.push_back(deviceGraphicQueueCreateInfo);

    if (context->gpuDetails.hasSeparatePresentQueue)
    {
        VkDeviceQueueCreateInfo devicePresentQueueCreateInfo = {};
        devicePresentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        devicePresentQueueCreateInfo.pNext = nullptr;
        devicePresentQueueCreateInfo.flags = 0;
        devicePresentQueueCreateInfo.queueFamilyIndex = context->gpuDetails.presentFamilyIndex;
        devicePresentQueueCreateInfo.queueCount = 1;
        devicePresentQueueCreateInfo.pQueuePriorities = queuePriorities.data();

        deviceQueueCreateInfos.push_back(devicePresentQueueCreateInfo);
    }

    // As we are using texture sampler, we need to enable this as a device feature.
    // This have many VkBool32 properties, leave it to VK_FALSE right now.
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = nullptr;
    deviceCreateInfo.flags = 0;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(context->deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = context->deviceExtensions.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    VkResult vkResult = vkCreateDevice(context->gpuDetails.gpu, &deviceCreateInfo, VK_NULL_HANDLE, &(context->device));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    // Create the graphic queue using graphicsFamilyIndex for given physical device.
    vkGetDeviceQueue(context->device, context->gpuDetails.graphicsFamilyIndex, 0, &(context->graphicsQueue));

    if (!context->gpuDetails.hasSeparatePresentQueue)
    {
        context->presentQueue = context->graphicsQueue;
    }
    else
    {
        vkGetDeviceQueue(context->device, context->gpuDetails.presentFamilyIndex, 0, &(context->presentQueue));
    }

    return VK_SUCCESS;
}

XR_API VkResult xrDestroyDevice(XrContext *context)
{
    vkDestroyDevice(context->device, VK_NULL_HANDLE);
    context->device = VK_NULL_HANDLE;

    return VK_SUCCESS;
}

VkResult xrQuerySwapchainSupportDetails(XrContext *context, VkPhysicalDevice gpu, XrSwapchainSupportDetails *details)
{
    VkResult vkResult = VK_SUCCESS;

    uint32_t formatCount = 0;
    uint32_t presentModeCount = 0;

    vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, context->surface, &(details->surfaceCapabilities));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, context->surface, &formatCount, nullptr);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    details->surfaceFormats.resize(formatCount);

    vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, context->surface, &formatCount, details->surfaceFormats.data());
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, context->surface, &presentModeCount, nullptr);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    details->presentModes.resize(presentModeCount);

    vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, context->surface, &presentModeCount, details->presentModes.data());
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

VkSurfaceFormatKHR xrChooseSurfaceFormat(XrContext *context, const std::vector<VkSurfaceFormatKHR> &surfaceFormats)
{
    xrPrintSurfaceFormatsDetails(context, surfaceFormats);

    if (surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        VkSurfaceFormatKHR surfaceFormat = {};
        surfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
        surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        return surfaceFormat;
    }

    for (const VkSurfaceFormatKHR &nextSurfaceFormat : surfaceFormats)
    {
        if (nextSurfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && nextSurfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return nextSurfaceFormat;
        }
    }

    return surfaceFormats[0];
}

VkPresentModeKHR xrChoosePresentMode(const std::vector<VkPresentModeKHR> &presentModes)
{
    VkPresentModeKHR defaultPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    for (const VkPresentModeKHR &nextPresentMode : presentModes)
    {
        // If nextPresentMode is VK_PRESENT_MODE_MAILBOX_KHR then use this as this is the best.
        if (nextPresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return nextPresentMode;
        }

        // If VK_PRESENT_MODE_MAILBOX_KHR was not found then use VK_PRESENT_MODE_IMMEDIATE_KHR.
        if (nextPresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            defaultPresentMode = nextPresentMode;
        }
    }

    return defaultPresentMode;
}

void xrChooseSurfaceExtent(VkSurfaceCapabilitiesKHR surfaceCapabilities, VkExtent2D *initialSurfaceExtent)
{
    if (surfaceCapabilities.currentExtent.width < UINT32_MAX)
    {
        initialSurfaceExtent->width = surfaceCapabilities.currentExtent.width;
        initialSurfaceExtent->height = surfaceCapabilities.currentExtent.height;
    }
    else
    {
        if (initialSurfaceExtent->width > surfaceCapabilities.maxImageExtent.width)
        {
            initialSurfaceExtent->width = surfaceCapabilities.maxImageExtent.width;
        }

        if (initialSurfaceExtent->width < surfaceCapabilities.minImageExtent.width)
        {
            initialSurfaceExtent->width = surfaceCapabilities.minImageExtent.width;
        }

        if (initialSurfaceExtent->height > surfaceCapabilities.maxImageExtent.height)
        {
            initialSurfaceExtent->height = surfaceCapabilities.maxImageExtent.height;
        }

        if (initialSurfaceExtent->height < surfaceCapabilities.minImageExtent.height)
        {
            initialSurfaceExtent->height = surfaceCapabilities.minImageExtent.height;
        }
    }
}

XR_API VkResult xrInitSwapchain(XrContext *context)
{
    VkExtent2D initialSurfaceExtent = {};
    initialSurfaceExtent.width = context->surfaceSize.width;
    initialSurfaceExtent.height = context->surfaceSize.height;

    xrQuerySwapchainSupportDetails(context, context->gpuDetails.gpu, &(context->swapchainSupportDetails));

    if (!context->swapchainSupportDetails.surfaceFormats.size())
    {
        assert(0 && "Surface format missing.");
        std::exit(EXIT_FAILURE);
    }

    context->surfaceFormat = xrChooseSurfaceFormat(context, context->swapchainSupportDetails.surfaceFormats);
    xrChooseSurfaceExtent(context->swapchainSupportDetails.surfaceCapabilities, &initialSurfaceExtent);

    context->surfaceSize.width = initialSurfaceExtent.width;
    context->surfaceSize.height = initialSurfaceExtent.height;

    VkPresentModeKHR presentMode = xrChoosePresentMode(context->swapchainSupportDetails.presentModes);

    // surfaceCapabilities.maxImageCount can be 0.
    // In this case the implementation supports unlimited amount of swap-chain images, limited by memory.
    // The amount of swap-chain images can also be fixed.

    uint32_t imageCount = context->swapchainSupportDetails.surfaceCapabilities.minImageCount + 1;

    if (context->swapchainSupportDetails.surfaceCapabilities.maxImageCount > 0 &&
        imageCount > context->swapchainSupportDetails.surfaceCapabilities.maxImageCount)
    {
        imageCount = context->swapchainSupportDetails.surfaceCapabilities.maxImageCount;
    }

    context->swapchainImageCount = imageCount;

    xrPrintSwapChainImageCount(
        context,
        context->swapchainSupportDetails.surfaceCapabilities.minImageCount,
        context->swapchainSupportDetails.surfaceCapabilities.maxImageCount,
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

        XR_LOG_INFO(context->logger, "---------- Presentation Mode End----------");
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = context->surface;
    swapchainCreateInfo.minImageCount = context->swapchainImageCount;
    swapchainCreateInfo.imageFormat = context->surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = context->surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent.width = context->surfaceSize.width;
    swapchainCreateInfo.imageExtent.height = context->surfaceSize.height;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.preTransform = context->swapchainSupportDetails.surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    if (context->gpuDetails.hasSeparatePresentQueue)
    {
        std::vector<uint32_t> indices = {context->gpuDetails.graphicsFamilyIndex, context->gpuDetails.presentFamilyIndex};

        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(indices.size()); // Ignored if imageSharingMode is VK_SHARING_MODE_EXCLUSIVE
        swapchainCreateInfo.pQueueFamilyIndices = indices.data();
    }
    else
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;     // Ignored if imageSharingMode is VK_SHARING_MODE_EXCLUSIVE
        swapchainCreateInfo.pQueueFamilyIndices = nullptr; // Ignored if imageSharingMode is VK_SHARING_MODE_EXCLUSIVE
    }

    VkResult vkResult = vkCreateSwapchainKHR(context->device, &swapchainCreateInfo, nullptr, &(context->swapchain));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = vkGetSwapchainImagesKHR(context->device, context->swapchain, &(context->swapchainImageCount), nullptr);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    context->swapchainImages.resize(context->swapchainImageCount);
    vkResult = vkGetSwapchainImagesKHR(context->device, context->swapchain, &(context->swapchainImageCount), context->swapchainImages.data());
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

XR_API VkResult xrDestroySwapchain(XrContext *context)
{
    vkDestroySwapchainKHR(context->device, context->swapchain, nullptr);
    context->swapchain = VK_NULL_HANDLE;
    context->swapchainImages.clear();
    return VK_SUCCESS;
}

XR_API VkResult xrInitSwapchainImageViews(XrContext *context)
{
    context->swapchainImageViews.resize(context->swapchainImageCount);

    for (uint32_t counter = 0; counter < context->swapchainImageCount; ++counter)
    {
        xrCreateImageView(
            context, context->swapchainImages[counter], context->surfaceFormat.format, context->swapchainImageViews[counter], VK_IMAGE_ASPECT_COLOR_BIT, 1
        );
    }

    return VK_SUCCESS;
}

XR_API VkResult xrDestroySwapchainImageViews(XrContext *context)
{
    for (VkImageView imageView : context->swapchainImageViews)
    {
        vkDestroyImageView(context->device, imageView, nullptr);
    }

    context->swapchainImageViews.clear();

    return VK_SUCCESS;
}

XR_API VkResult xrCreateShaderModule(XrContext *context, const std::vector<char> &code, VkShaderModule *shaderModule)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pNext = nullptr;
    shaderModuleCreateInfo.flags = 0;
    shaderModuleCreateInfo.codeSize = code.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    return vkCreateShaderModule(context->device, &shaderModuleCreateInfo, nullptr, shaderModule);
}

XR_API VkResult xrInitGraphicsPiplineCache(XrContext *context)
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCacheCreateInfo.pNext = NULL;
    pipelineCacheCreateInfo.flags = 0;
    pipelineCacheCreateInfo.initialDataSize = 0;
    pipelineCacheCreateInfo.pInitialData = NULL;

    VkResult vkResult = vkCreatePipelineCache(context->device, &pipelineCacheCreateInfo, nullptr, &(context->pipelineCache));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

XR_API VkResult xrDestroyGraphicsPiplineCache(XrContext *context)
{
    vkDestroyPipelineCache(context->device, context->pipelineCache, nullptr);
    context->pipelineCache = VK_NULL_HANDLE;
    return VK_SUCCESS;
}

XR_API VkResult xrInitGraphicsPipline(XrContext *context)
{
    VkResult vkResult = VK_SUCCESS;
    std::vector<char> vertexShaderCode;
    std::vector<char> fragmentShaderCode;

    if (!xrReadFile(context->vertexShaderFilePath, &vertexShaderCode))
    {
        XR_LOG_ERROR(context->logger, "Cannot open vertex shader file: %s", context->vertexShaderFilePath);
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (!xrReadFile(context->fragmentShaderFile, &fragmentShaderCode))
    {
        XR_LOG_ERROR(context->logger, "Cannot open fragment shader file: %s", context->fragmentShaderFile);
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkShaderModule vertexShaderModule;
    memset((void *)&vertexShaderModule, 0, sizeof(VkShaderModule));

    vkResult = xrCreateShaderModule(context, vertexShaderCode, &vertexShaderModule);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkShaderModule fragmentShaderModule;
    memset((void *)&fragmentShaderModule, 0, sizeof(VkShaderModule));

    vkResult = xrCreateShaderModule(context, fragmentShaderCode, &fragmentShaderModule);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {};
    vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageCreateInfo.pNext = nullptr;
    vertexShaderStageCreateInfo.flags = 0;
    vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageCreateInfo.module = vertexShaderModule;
    vertexShaderStageCreateInfo.pName = "main";
    vertexShaderStageCreateInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {};
    fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageCreateInfo.pNext = nullptr;
    fragmentShaderStageCreateInfo.flags = 0;
    fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStageCreateInfo.module = fragmentShaderModule;
    fragmentShaderStageCreateInfo.pName = "main";
    fragmentShaderStageCreateInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo shaderStageCreateInfos[] = {vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo};

    VkVertexInputBindingDescription vertexBindingDescription = XrVertex::xrGetBindingDescription();
    std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescription = XrVertex::xrGetAttributeDescription();

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.pNext = nullptr;
    vertexInputStateCreateInfo.flags = 0;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescription.size());
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescription.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
    inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.pNext = nullptr;
    inputAssemblyStateCreateInfo.flags = 0;
    inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)(context->surfaceSize.width);
    viewport.height = (float)(context->surfaceSize.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent.width = context->surfaceSize.width;
    scissor.extent.height = context->surfaceSize.height;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.pNext = nullptr;
    viewportStateCreateInfo.flags = 0;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.pNext = nullptr;
    rasterizationStateCreateInfo.flags = 0;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
    rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationStateCreateInfo.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.pNext = nullptr;
    multisampleStateCreateInfo.flags = 0;
    multisampleStateCreateInfo.rasterizationSamples = context->gpuDetails.msaaSamples;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.minSampleShading = 1.0f;
    multisampleStateCreateInfo.pSampleMask = nullptr;
    multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
    depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCreateInfo.pNext = nullptr;
    depthStencilStateCreateInfo.flags = 0;
    depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.front = {};
    depthStencilStateCreateInfo.back = {};
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
    colorBlendingStateCreateInfo.pNext = nullptr;
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
    // std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };
    std::vector<VkDynamicState> dynamicStates = {};

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.pNext = nullptr;
    dynamicStateCreateInfo.flags = 0;
    dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &(context->descriptorSetLayout);
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = 0;

    vkResult = vkCreatePipelineLayout(context->device, &pipelineLayoutCreateInfo, nullptr, &(context->pipelineLayout));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext = nullptr;
    pipelineCreateInfo.flags = 0;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStageCreateInfos;
    pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    pipelineCreateInfo.pTessellationState = nullptr;
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

    vkResult = vkCreateGraphicsPipelines(context->device, context->pipelineCache, 1, &pipelineCreateInfo, nullptr, &(context->pipeline));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkDestroyShaderModule(context->device, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(context->device, vertexShaderModule, nullptr);

    return vkResult;
}

XR_API VkResult xrDestroyGraphicsPipline(XrContext *context)
{
    vkDestroyPipeline(context->device, context->pipeline, nullptr);
    vkDestroyPipelineLayout(context->device, context->pipelineLayout, nullptr);
    context->pipeline = VK_NULL_HANDLE;
    context->pipelineLayout = VK_NULL_HANDLE;
    return VK_SUCCESS;
}

VkFormat xrFindSupportedFormat(
    VkPhysicalDevice gpu,
    const std::vector<VkFormat> &formatsToCheck,
    VkImageTiling imageTiling,
    VkFormatFeatureFlags formatFeatureFlags
)
{
    for (VkFormat nextFormat : formatsToCheck)
    {
        VkFormatProperties formatProperties = {};
        vkGetPhysicalDeviceFormatProperties(gpu, nextFormat, &formatProperties);

        if (imageTiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & formatFeatureFlags) == formatFeatureFlags)
        {
            return nextFormat;
        }

        if (imageTiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & formatFeatureFlags) == formatFeatureFlags)
        {
            return nextFormat;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

VkFormat xrFindDepthFormat(XrContext *context)
{
    std::vector<VkFormat> formatsToCheck = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    return xrFindSupportedFormat(context->gpuDetails.gpu, formatsToCheck, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool xrHasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || false;
}

XR_API VkResult xrInitDepthStencilImage(XrContext *context)
{
    VkFormat depthStencilFormat = xrFindDepthFormat(context);
    if (depthStencilFormat == VK_FORMAT_UNDEFINED)
    {
        assert(0 && "Depth stencil format not selected.");
    }

    xrCreateImage(
        context,
        context->surfaceSize.width,
        context->surfaceSize.height,
        1,
        context->gpuDetails.msaaSamples,
        depthStencilFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        context->depthImage,
        context->depthImageMemory
    );

    xrCreateImageView(context, context->depthImage, depthStencilFormat, context->depthImageView, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    return VK_SUCCESS;
}

XR_API VkResult xrDestroyDepthStencilImage(XrContext *context)
{
    vkDestroyImageView(context->device, context->depthImageView, nullptr);
    vkDestroyImage(context->device, context->depthImage, nullptr);
    vkFreeMemory(context->device, context->depthImageMemory, nullptr);
    context->depthImageView = VK_NULL_HANDLE;
    context->depthImage = VK_NULL_HANDLE;
    context->depthImageMemory = VK_NULL_HANDLE;
    return VK_SUCCESS;
}

XR_API VkResult xrInitMSAAColorImage(XrContext *context)
{
    xrCreateImage(
        context,
        context->surfaceSize.width,
        context->surfaceSize.height,
        1,
        context->gpuDetails.msaaSamples,
        context->surfaceFormat.format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        context->msaaColorImage,
        context->msaaColorImageMemory
    );

    xrCreateImageView(context, context->msaaColorImage, context->surfaceFormat.format, context->msaaColorImageView, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    return VK_SUCCESS;
}

XR_API VkResult xrDestroyMSAAColorImage(XrContext *context)
{
    vkDestroyImageView(context->device, context->msaaColorImageView, nullptr);
    vkDestroyImage(context->device, context->msaaColorImage, nullptr);
    vkFreeMemory(context->device, context->msaaColorImageMemory, nullptr);
    context->msaaColorImageView = VK_NULL_HANDLE;
    context->msaaColorImage = VK_NULL_HANDLE;
    context->msaaColorImageMemory = VK_NULL_HANDLE;
    return VK_SUCCESS;
}

XR_API VkResult xrInitRenderPass(XrContext *context)
{
    VkAttachmentDescription colorAttachmentDescription = {};
    colorAttachmentDescription.flags = 0;
    colorAttachmentDescription.format = context->surfaceFormat.format;
    colorAttachmentDescription.samples = context->gpuDetails.msaaSamples;
    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthStencilAttachmentDescription = {};
    depthStencilAttachmentDescription.flags = 0;
    depthStencilAttachmentDescription.format = xrFindDepthFormat(context);
    depthStencilAttachmentDescription.samples = context->gpuDetails.msaaSamples;
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
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;
    subpassDescription.pResolveAttachments = &colorAttachmentResolveReference;
    subpassDescription.pDepthStencilAttachment = &depthStencilAttachmentReference;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;

    std::array<VkAttachmentDescription, 3> attachments = {colorAttachmentDescription, depthStencilAttachmentDescription, colorAttachmentResolveDescription};

    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    VkResult vkResult = vkCreateRenderPass(context->device, &renderPassCreateInfo, nullptr, &(context->renderPass));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

XR_API VkResult xrDestroyRenderPass(XrContext *context)
{
    vkDestroyRenderPass(context->device, context->renderPass, nullptr);
    context->renderPass = VK_NULL_HANDLE;
    return VK_SUCCESS;
}

XR_API VkResult xrInitDescriptorSetLayout(XrContext *context)
{
    VkDescriptorSetLayoutBinding uboDescriptorSetLayoutBinding = {};
    uboDescriptorSetLayoutBinding.binding = 0;
    uboDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboDescriptorSetLayoutBinding.descriptorCount = 1;
    uboDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerDescriptorSetLayoutBinding = {};
    samplerDescriptorSetLayoutBinding.binding = 1;
    samplerDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerDescriptorSetLayoutBinding.descriptorCount = 1;
    samplerDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings = {uboDescriptorSetLayoutBinding, samplerDescriptorSetLayoutBinding};

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.flags = 0;
    descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    descriptorSetLayoutCreateInfo.pBindings = layoutBindings.data();

    VkResult vkResult = vkCreateDescriptorSetLayout(context->device, &descriptorSetLayoutCreateInfo, nullptr, &context->descriptorSetLayout);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

XR_API VkResult xrDestroyDescriptorSetLayout(XrContext *context)
{
    vkDestroyDescriptorSetLayout(context->device, context->descriptorSetLayout, nullptr);
    context->descriptorSetLayout = VK_NULL_HANDLE;
    return VK_SUCCESS;
}

XR_API VkResult xrInitFrameBuffers(XrContext *context)
{
    VkResult vkResult = VK_SUCCESS;
    context->framebuffers.resize(context->swapchainImageCount);

    for (uint32_t swapchainImageCounter = 0; swapchainImageCounter < context->swapchainImageCount; ++swapchainImageCounter)
    {
        std::array<VkImageView, 3> attachments = {context->msaaColorImageView, context->depthImageView, context->swapchainImageViews[swapchainImageCounter]};

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = nullptr;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = context->renderPass;
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data();
        framebufferCreateInfo.width = context->surfaceSize.width;
        framebufferCreateInfo.height = context->surfaceSize.height;
        framebufferCreateInfo.layers = 1;

        vkResult = vkCreateFramebuffer(context->device, &framebufferCreateInfo, nullptr, &(context->framebuffers[swapchainImageCounter]));
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }
    }

    return vkResult;
}

XR_API VkResult xrDestroyFrameBuffers(XrContext *context)
{
    for (VkFramebuffer nextFrameBuffer : context->framebuffers)
    {
        vkDestroyFramebuffer(context->device, nextFrameBuffer, nullptr);
    }

    context->framebuffers.clear();
    return VK_SUCCESS;
}

XR_API VkResult xrInitCommandPool(XrContext *context)
{
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = nullptr;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = context->gpuDetails.graphicsFamilyIndex;

    VkResult vkResult = vkCreateCommandPool(context->device, &commandPoolCreateInfo, nullptr, &(context->commandPool));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

XR_API VkResult xrDestroyCommandPool(XrContext *context)
{
    vkDestroyCommandPool(context->device, context->commandPool, nullptr);
    context->commandPool = VK_NULL_HANDLE;
    return VK_SUCCESS;
}

XR_API VkResult xrCreateImage(
    XrContext *context,
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
)
{
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = nullptr;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = samplesCount;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.usage = usage;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0;
    imageCreateInfo.pQueueFamilyIndices = nullptr;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult vkResult = vkCreateImage(context->device, &imageCreateInfo, nullptr, &image);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkMemoryRequirements imageMemoryRequirements = {};
    vkGetImageMemoryRequirements(context->device, image, &imageMemoryRequirements);

    uint32_t memoryIndex = xrFindMemoryTypeIndex(&(context->gpuDetails.memoryProperties), &imageMemoryRequirements, memoryPropertyFlags);

    VkMemoryAllocateInfo memoryAllocationInfo = {};
    memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocationInfo.pNext = nullptr;
    memoryAllocationInfo.allocationSize = imageMemoryRequirements.size;
    memoryAllocationInfo.memoryTypeIndex = memoryIndex;

    vkResult = vkAllocateMemory(context->device, &memoryAllocationInfo, nullptr, &imageMemory);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkBindImageMemory(context->device, image, imageMemory, 0);
    return VK_SUCCESS;
}

XR_API VkResult
xrCreateImageView(XrContext *context, VkImage image, VkFormat format, VkImageView &imageView, VkImageAspectFlags imageAspectFlags, uint32_t mipLevels)
{
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = nullptr;
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

    VkResult vkResult = vkCreateImageView(context->device, &imageViewCreateInfo, nullptr, &imageView);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

XR_API VkResult xrInitTextureImage(XrContext *context, XrModel *model, XrTexture *texture, void *pixels)
{
    VkDeviceSize deviceSize = texture->width * texture->height * 4;
    VkBuffer stagingImageBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingImageBufferMemory = VK_NULL_HANDLE;

    xrCreateBuffer(
        context,
        deviceSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingImageBuffer,
        &stagingImageBufferMemory
    );

    void *data = nullptr;
    vkMapMemory(context->device, stagingImageBufferMemory, 0, deviceSize, 0, &data);
    memcpy(data, pixels, deviceSize);
    vkUnmapMemory(context->device, stagingImageBufferMemory);

    xrCreateImage(
        context,
        static_cast<uint32_t>(texture->width),
        static_cast<uint32_t>(texture->height),
        texture->mipLevels,
        VK_SAMPLE_COUNT_1_BIT,
        texture->format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        model->textureImage,
        model->textureImageMemory
    );

    xrTransitionImageLayout(context, model->textureImage, texture->format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture->mipLevels);

    xrCopyBufferToImage(context, stagingImageBuffer, model->textureImage, static_cast<uint32_t>(texture->width), static_cast<uint32_t>(texture->height));

    // Generate the mipmaps images and then transition image layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
    xrGenerateMipmaps(context, model->textureImage, texture->width, texture->height, texture->mipLevels);

    vkDestroyBuffer(context->device, stagingImageBuffer, nullptr);
    vkFreeMemory(context->device, stagingImageBufferMemory, nullptr);
    return VK_SUCCESS;
}

XR_API VkResult xrDestroyTextureImage(XrContext *context, XrModel *model)
{
    vkDestroyImage(context->device, model->textureImage, nullptr);
    vkFreeMemory(context->device, model->textureImageMemory, nullptr);
    model->textureImage = VK_NULL_HANDLE;
    model->textureImageMemory = VK_NULL_HANDLE;
    return VK_SUCCESS;
}

VkResult xrGenerateMipmaps(XrContext *context, VkImage &image, int32_t textureWidth, int32_t textureHeight, uint32_t mipLevels)
{
    VkResult vkResult = VK_SUCCESS;
    int32_t mipWidth = textureWidth;
    int32_t mipHeight = textureHeight;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkResult = xrBeginOneTimeCommand(context, commandBuffer);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = nullptr;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;
    imageMemoryBarrier.subresourceRange.levelCount = 1;

    // mip level starts from 1 not 0.
    for (uint32_t counter = 1; counter < mipLevels; ++counter)
    {
        // First, we transition level counter - 1 to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL.
        // This transition will wait for level counter - 1 to be filled, either from the previous blit command,
        // or from vkCmdCopyBufferToImage
        imageMemoryBarrier.subresourceRange.baseMipLevel = counter - 1;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

        VkImageBlit imageBlit = {};

        // srcOffsets array determine the 3D region that data will be blitted from.
        imageBlit.srcOffsets[0] = {0, 0, 0};

        // The z dimension of srcOffsets[1] must be 1, since a 2D image has a depth of 1
        imageBlit.srcOffsets[1] = {mipWidth, mipHeight, 1};
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
        imageBlit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        // The destination mip level is counter
        imageBlit.dstSubresource.mipLevel = counter;
        imageBlit.dstSubresource.baseArrayLayer = 0;
        imageBlit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(
            commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR
        );

        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        // This barrier transitions mip level i - 1 to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
        // This transition waits on the current blit command to finish.
        // All sampling operations will wait on this transition to finish.
        vkCmdPipelineBarrier(
            commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier
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
    imageMemoryBarrier.subresourceRange.baseMipLevel = mipLevels - 1;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier
    );

    // Now end the command buffer.
    vkResult = xrEndOneTimeCommand(context, commandBuffer);

    return vkResult;
}

XR_API VkResult xrInitTextureImageView(XrContext *context, XrModel *model, XrTexture *texture)
{
    xrCreateImageView(context, model->textureImage, texture->format, model->textureImageView, VK_IMAGE_ASPECT_COLOR_BIT, texture->mipLevels);
    return VK_SUCCESS;
}

XR_API VkResult xrDestroyTextureImageView(XrContext *context, XrModel *model)
{
    vkDestroyImageView(context->device, model->textureImageView, nullptr);
    model->textureImageView = VK_NULL_HANDLE;
    return VK_SUCCESS;
}

XR_API VkResult xrInitTextureSampler(XrContext *context, XrModel *model, XrTexture *texture)
{
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.pNext = nullptr;
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

    VkResult vkResult = vkCreateSampler(context->device, &samplerCreateInfo, nullptr, &(model->textureSampler));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

XR_API VkResult xrDestroyTextureSampler(XrContext *context, XrModel *model)
{
    vkDestroySampler(context->device, model->textureSampler, nullptr);
    model->textureSampler = VK_NULL_HANDLE;
    return VK_SUCCESS;
}

XR_API VkResult xrCreateBuffer(
    XrContext *context,
    VkDeviceSize size,
    VkBufferUsageFlags bufferUsage,
    VkMemoryPropertyFlags memoryProperties,
    VkBuffer *buffer,
    VkDeviceMemory *bufferMemory
)
{
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.pNext = nullptr;
    bufferCreateInfo.flags = 0;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = bufferUsage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = 0;
    bufferCreateInfo.pQueueFamilyIndices = nullptr; // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT

    VkResult vkResult = vkCreateBuffer(context->device, &bufferCreateInfo, nullptr, buffer);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkMemoryRequirements bufferMemoryRequirements = {};
    vkGetBufferMemoryRequirements(context->device, *buffer, &bufferMemoryRequirements);

    uint32_t memoryIndex = xrFindMemoryTypeIndex(&(context->gpuDetails.memoryProperties), &bufferMemoryRequirements, memoryProperties);

    if (XR_CHECK_RESULT(memoryIndex, UINT32_MAX))
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkMemoryAllocateInfo memoryAllocationInfo = {};
    memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocationInfo.pNext = nullptr;
    memoryAllocationInfo.allocationSize = bufferMemoryRequirements.size;
    memoryAllocationInfo.memoryTypeIndex = memoryIndex;

    vkResult = vkAllocateMemory(context->device, &memoryAllocationInfo, nullptr, bufferMemory);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = vkBindBufferMemory(context->device, *buffer, *bufferMemory, 0);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

XR_API VkResult xrBeginOneTimeCommand(XrContext *context, VkCommandBuffer &commandBuffer)
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = context->commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VkResult vkResult = vkAllocateCommandBuffers(context->device, &commandBufferAllocateInfo, &commandBuffer);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    return vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
}

XR_API VkResult xrEndOneTimeCommand(XrContext *context, VkCommandBuffer &commandBuffer)
{
    VkResult vkResult = vkEndCommandBuffer(commandBuffer);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;

    vkResult = vkQueueSubmit(context->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkQueueWaitIdle(context->graphicsQueue);
    vkFreeCommandBuffers(context->device, context->commandPool, 1, &commandBuffer);

    return vkResult;
}

VkResult xrTransitionImageLayout(
    XrContext *context,
    VkImage image,
    VkFormat format,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    uint32_t mipLevels
)
{
    VkResult vkResult = VK_SUCCESS;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkResult = xrBeginOneTimeCommand(context, commandBuffer);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = nullptr;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;
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
        return VK_ERROR_NOT_PERMITTED;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStageMask, destinationStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

    vkResult = xrEndOneTimeCommand(context, commandBuffer);

    return vkResult;
}

XR_API VkResult xrCopyBuffer(XrContext *context, VkBuffer sourceBuffer, VkBuffer targetBuffer, VkDeviceSize size)
{
    VkResult vkResult = VK_SUCCESS;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkResult = xrBeginOneTimeCommand(context, commandBuffer);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    vkCmdCopyBuffer(commandBuffer, sourceBuffer, targetBuffer, 1, &copyRegion);

    vkResult = xrEndOneTimeCommand(context, commandBuffer);

    return vkResult;
}

XR_API VkResult xrCopyBufferToImage(XrContext *context, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkResult vkResult = VK_SUCCESS;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkResult = xrBeginOneTimeCommand(context, commandBuffer);

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
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    vkResult = xrEndOneTimeCommand(context, commandBuffer);

    return vkResult;
}

XR_API VkResult xrInitVertexBuffer(XrContext *context, XrModel *model)
{
    VkResult vkResult = VK_SUCCESS;
    VkDeviceSize size = sizeof(model->vertices[0]) * model->vertices.size();
    VkBufferUsageFlags stagingBufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags stagingMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    vkResult = xrCreateBuffer(context, size, stagingBufferUsage, stagingMemoryProperties, &stagingBuffer, &stagingBufferMemory);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    void *stagingBufferData = nullptr;
    vkResult = vkMapMemory(context->device, stagingBufferMemory, 0, size, 0, &stagingBufferData);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    memcpy(stagingBufferData, model->vertices.data(), (size_t)size);
    vkUnmapMemory(context->device, stagingBufferMemory);

    VkBufferUsageFlags vertexBufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VkMemoryPropertyFlags vertexMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    vkResult = xrCreateBuffer(context, size, vertexBufferUsage, vertexMemoryProperties, &(model->vertexBuffer), &(model->vertexBufferMemory));

    if (XR_IS_ERROR(vkResult))
    {
        vkFreeMemory(context->device, stagingBufferMemory, nullptr);
        return vkResult;
    }

    vkResult = xrCopyBuffer(context, stagingBuffer, model->vertexBuffer, size);

    if (XR_IS_ERROR(vkResult))
    {
        vkFreeMemory(context->device, stagingBufferMemory, nullptr);
        vkDestroyBuffer(context->device, stagingBuffer, nullptr);
        return vkResult;
    }

    vkDestroyBuffer(context->device, stagingBuffer, nullptr);
    return vkResult;
}

XR_API VkResult xrDestroyVertexBuffer(XrContext *context, XrModel *model)
{
    vkDestroyBuffer(context->device, model->vertexBuffer, nullptr);
    vkFreeMemory(context->device, model->vertexBufferMemory, nullptr);
    model->vertexBuffer = VK_NULL_HANDLE;
    model->vertexBufferMemory = VK_NULL_HANDLE;
    return VK_SUCCESS;
}

XR_API VkResult xrInitIndexBuffer(XrContext *context, XrModel *model)
{
    VkResult vkResult = VK_SUCCESS;

    VkDeviceSize size = sizeof(model->vertexIndices[0]) * model->vertexIndices.size();
    VkBufferUsageFlags stagingBufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags stagingMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    xrCreateBuffer(context, size, stagingBufferUsage, stagingMemoryProperties, &stagingBuffer, &stagingBufferMemory);

    void *stagingBufferData = nullptr;
    vkResult = vkMapMemory(context->device, stagingBufferMemory, 0, size, 0, &stagingBufferData);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    memcpy(stagingBufferData, model->vertexIndices.data(), (size_t)size);
    vkUnmapMemory(context->device, stagingBufferMemory);

    VkBufferUsageFlags indexBufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    VkMemoryPropertyFlags indexMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    xrCreateBuffer(context, size, indexBufferUsage, indexMemoryProperties, &(model->indexBuffer), &(model->indexBufferMemory));
    xrCopyBuffer(context, stagingBuffer, model->indexBuffer, size);
    vkDestroyBuffer(context->device, stagingBuffer, nullptr);
    vkFreeMemory(context->device, stagingBufferMemory, nullptr);

    return vkResult;
}

XR_API VkResult xrDestroyIndexBuffer(XrContext *context, XrModel *model)
{
    vkDestroyBuffer(context->device, model->indexBuffer, nullptr);
    vkFreeMemory(context->device, model->indexBufferMemory, nullptr);
    model->indexBuffer = VK_NULL_HANDLE;
    model->indexBufferMemory = VK_NULL_HANDLE;
    return VK_SUCCESS;
}

XR_API VkResult xrInitUniformBuffers(XrContext *context, XrModel *model)
{
    VkDeviceSize size = sizeof(XrUniformBufferObject);
    VkBufferUsageFlags uniformBufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    VkMemoryPropertyFlags uniformMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    model->uniformBuffers.resize(context->swapchainImages.size());
    model->uniformBuffersMemory.resize(context->swapchainImages.size());

    for (size_t counter = 0; counter < context->swapchainImages.size(); ++counter)
    {
        xrCreateBuffer(context, size, uniformBufferUsage, uniformMemoryProperties, &(model->uniformBuffers[counter]), &(model->uniformBuffersMemory[counter]));
    }

    return VK_SUCCESS;
}

XR_API VkResult xrDestroyUniformBuffers(XrContext *context, XrModel *model)
{
    for (size_t counter = 0; counter < context->swapchainImages.size(); ++counter)
    {
        vkDestroyBuffer(context->device, model->uniformBuffers[counter], nullptr);
        vkFreeMemory(context->device, model->uniformBuffersMemory[counter], nullptr);
    }

    model->uniformBuffers.clear();
    model->uniformBuffersMemory.clear();
    return VK_SUCCESS;
}

XR_API VkResult xrInitDescriptorPool(XrContext *context, size_t models)
{
    VkDescriptorPoolSize uboPoolSize = {};
    uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboPoolSize.descriptorCount = static_cast<uint32_t>(context->swapchainImages.size() * models);

    VkDescriptorPoolSize samplerPoolSize = {};
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = static_cast<uint32_t>(context->swapchainImages.size() * models);
    ;

    std::array<VkDescriptorPoolSize, 2> poolSizes = {uboPoolSize, samplerPoolSize};

    VkDescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.pNext = nullptr;
    poolCreateInfo.flags = 0;
    // If you want to explicitly destroy the descriptorSet, then set this bit
    // else you will get runtime error while destroying the descriptorSet.
    // We are not going to used this for now.
    // poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolCreateInfo.maxSets = static_cast<uint32_t>(context->swapchainImages.size() * models);
    poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCreateInfo.pPoolSizes = poolSizes.data();

    VkResult vkResult = vkCreateDescriptorPool(context->device, &poolCreateInfo, nullptr, &(context->descriptorPool));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    return vkResult;
}

XR_API VkResult xrDestroyDescriptorPool(XrContext *context)
{
    vkDestroyDescriptorPool(context->device, context->descriptorPool, nullptr);
    context->descriptorPool = VK_NULL_HANDLE;
    return VK_SUCCESS;
}

XR_API VkResult xrInitDescriptorSets(XrContext *context, std::vector<XrModel *> models)
{
    VkResult vkResult = VK_SUCCESS;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(context->swapchainImages.size(), context->descriptorSetLayout);

    for (size_t index = 0; index < models.size(); ++index)
    {
        XrModel *model = models[index];
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.pNext = nullptr;
        descriptorSetAllocateInfo.descriptorPool = context->descriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(context->swapchainImages.size());
        descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts.data();

        model->descriptorSets.resize(context->swapchainImages.size());

        vkResult = vkAllocateDescriptorSets(context->device, &descriptorSetAllocateInfo, model->descriptorSets.data());
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        for (size_t counter = 0; counter < context->swapchainImages.size(); ++counter)
        {
            VkDescriptorBufferInfo descriptorBufferInfo = {};
            descriptorBufferInfo.buffer = model->uniformBuffers[counter];
            descriptorBufferInfo.offset = 0;
            descriptorBufferInfo.range = sizeof(XrUniformBufferObject);

            VkDescriptorImageInfo descriptorImageInfo = {};
            descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorImageInfo.imageView = model->textureImageView;
            descriptorImageInfo.sampler = model->textureSampler;

            VkWriteDescriptorSet uniformBudderDescriptorWrite = {};
            uniformBudderDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            uniformBudderDescriptorWrite.pNext = nullptr;
            uniformBudderDescriptorWrite.dstSet = model->descriptorSets[counter];
            uniformBudderDescriptorWrite.dstBinding = 0;
            uniformBudderDescriptorWrite.dstArrayElement = 0;
            uniformBudderDescriptorWrite.descriptorCount = 1;
            uniformBudderDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uniformBudderDescriptorWrite.pImageInfo = nullptr;
            uniformBudderDescriptorWrite.pBufferInfo = &descriptorBufferInfo;
            uniformBudderDescriptorWrite.pTexelBufferView = nullptr;

            VkWriteDescriptorSet textureImageDescriptorWrite = {};
            textureImageDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            textureImageDescriptorWrite.pNext = nullptr;
            textureImageDescriptorWrite.dstSet = model->descriptorSets[counter];
            textureImageDescriptorWrite.dstBinding = 1;
            textureImageDescriptorWrite.dstArrayElement = 0;
            textureImageDescriptorWrite.descriptorCount = 1;
            textureImageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            textureImageDescriptorWrite.pImageInfo = &descriptorImageInfo;
            textureImageDescriptorWrite.pBufferInfo = nullptr;
            textureImageDescriptorWrite.pTexelBufferView = nullptr;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites = {uniformBudderDescriptorWrite, textureImageDescriptorWrite};
            vkUpdateDescriptorSets(context->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    return vkResult;
}

XR_API VkResult xrDestroyDescriptorSets(XrContext *context, std::vector<XrModel *> models)
{
    for (size_t index = 0; index < models.size(); ++index)
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

        model->descriptorSets.clear();
    }

    return VK_SUCCESS;
}

XR_API VkResult xrInitCommandBuffers(XrContext *context, std::vector<XrModel *> models)
{
    VkResult vkResult = VK_SUCCESS;

    context->commandBuffers.resize(context->framebuffers.size());

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = context->commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(context->commandBuffers.size());

    vkResult = vkAllocateCommandBuffers(context->device, &commandBufferAllocateInfo, context->commandBuffers.data());
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    for (uint32_t counter = 0; counter < context->commandBuffers.size(); ++counter)
    {
        VkCommandBufferBeginInfo commandBufferBeginInfo = {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.pNext = nullptr;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        commandBufferBeginInfo.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(context->commandBuffers[counter], &commandBufferBeginInfo);

        VkRect2D renderArea = {};
        renderArea.offset.x = 0;
        renderArea.offset.y = 0;
        renderArea.extent.width = context->surfaceSize.width;
        renderArea.extent.height = context->surfaceSize.height;

        std::array<VkClearValue, 2> clearValue = {};
        clearValue[0].color = {0.0f, 0.0f, 0.0f, 1.0f}; // {r, g, b, a}
        clearValue[1].depthStencil = {1.0f, 0};         // {depth, stencil}

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = context->renderPass;
        renderPassBeginInfo.framebuffer = context->framebuffers[counter];
        renderPassBeginInfo.renderArea = renderArea;
        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValue.size());
        renderPassBeginInfo.pClearValues = clearValue.data();

        vkCmdBeginRenderPass(context->commandBuffers[counter], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(context->commandBuffers[counter], VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline);

        VkDeviceSize offset = {0};

        for (size_t index = 0; index < models.size(); ++index)
        {
            XrModel *model = models[index];
            vkCmdBindVertexBuffers(context->commandBuffers[counter], 0, 1, &(model->vertexBuffer), &offset);
            vkCmdBindIndexBuffer(context->commandBuffers[counter], model->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(
                context->commandBuffers[counter], VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipelineLayout, 0, 1, &(model->descriptorSets[counter]), 0, nullptr
            );

            vkCmdDrawIndexed(context->commandBuffers[counter], static_cast<uint32_t>(model->vertexIndices.size()), 1, 0, 0, 0);
        }

        vkCmdEndRenderPass(context->commandBuffers[counter]);

        VkResult vkResult = vkEndCommandBuffer(context->commandBuffers[counter]);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }
    }

    return vkResult;
}

XR_API VkResult xrDestroyCommandBuffers(XrContext *context)
{
    vkFreeCommandBuffers(context->device, context->commandPool, static_cast<uint32_t>(context->commandBuffers.size()), context->commandBuffers.data());
    context->commandBuffers.clear();
    return VK_SUCCESS;
}

XR_API VkResult xrInitSynchronizations(XrContext *context)
{
    context->imageAvailableSemaphores.resize(context->MAX_FRAMES_IN_FLIGHT);
    context->renderFinishedSemaphores.resize(context->MAX_FRAMES_IN_FLIGHT);
    context->inFlightFences.resize(context->MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t counter = 0; counter < context->MAX_FRAMES_IN_FLIGHT; ++counter)
    {
        VkResult vkResult = vkCreateSemaphore(context->device, &semaphoreCreateInfo, nullptr, &context->imageAvailableSemaphores[counter]);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        vkResult = vkCreateSemaphore(context->device, &semaphoreCreateInfo, nullptr, &context->renderFinishedSemaphores[counter]);
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }

        vkResult = vkCreateFence(context->device, &fenceCreateInfo, nullptr, &(context->inFlightFences[counter]));
        if (XR_IS_ERROR(vkResult))
        {
            return vkResult;
        }
    }

    return VK_SUCCESS;
}

XR_API VkResult xrDestroySynchronizations(XrContext *context)
{
    for (size_t counter = 0; counter < context->MAX_FRAMES_IN_FLIGHT; ++counter)
    {
        vkDestroySemaphore(context->device, context->imageAvailableSemaphores[counter], nullptr);
        vkDestroySemaphore(context->device, context->renderFinishedSemaphores[counter], nullptr);
        vkDestroyFence(context->device, context->inFlightFences[counter], nullptr);
    }

    context->imageAvailableSemaphores.clear();
    context->renderFinishedSemaphores.clear();
    context->inFlightFences.clear();

    return VK_SUCCESS;
}

XR_API VkResult xrRecreateSwapChain(XrContext *context, std::vector<XrModel *> models)
{
    XR_LOG_INFO(context->logger, "---------- Recreate SwapChain --------");
    xrCleanupSwapChain(context, models);
    xrInitSwapchain(context);
    xrInitSwapchainImageViews(context);
    xrInitRenderPass(context);
    xrInitGraphicsPiplineCache(context);
    xrInitGraphicsPipline(context);
    xrInitDepthStencilImage(context);
    xrInitMSAAColorImage(context);
    xrInitFrameBuffers(context);

    for (size_t index = 0; index < models.size(); ++index)
    {
        xrInitUniformBuffers(context, models[index]);
    }

    xrInitDescriptorPool(context, models.size());
    xrInitDescriptorSets(context, models);
    xrInitCommandBuffers(context, models);
    xrInitSynchronizations(context);
    return VK_SUCCESS;
}

XR_API VkResult xrCleanupSwapChain(XrContext *context, std::vector<XrModel *> models)
{
    xrWaitForIdle(context);
    xrDestroySynchronizations(context);
    xrDestroyCommandBuffers(context);
    xrDestroyDescriptorSets(context, models);
    xrDestroyDescriptorPool(context);

    for (size_t index = 0; index < models.size(); ++index)
    {
        xrDestroyUniformBuffers(context, models[index]);
    }

    xrDestroyFrameBuffers(context);
    xrDestroyMSAAColorImage(context);
    xrDestroyDepthStencilImage(context);
    xrDestroyGraphicsPipline(context);
    xrDestroyGraphicsPiplineCache(context);
    xrDestroyRenderPass(context);
    xrDestroySwapchainImageViews(context);
    xrDestroySwapchain(context);
    return VK_SUCCESS;
}

XR_API VkResult xrRender(XrContext *context, std::vector<XrModel *> models)
{
    VkResult vkResult = VK_SUCCESS;

    // Update the current frame count at start as we might return in between and fail to update the counter
    context->currentFrame = (context->currentFrame + 1) % context->MAX_FRAMES_IN_FLIGHT;

    vkResult = vkWaitForFences(context->device, 1, &(context->inFlightFences[context->currentFrame]), VK_TRUE, UINT64_MAX);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    uint32_t activeSwapchainImageId = UINT32_MAX;

    vkResult = vkAcquireNextImageKHR(
        context->device, context->swapchain, UINT64_MAX, context->imageAvailableSemaphores[context->currentFrame], VK_NULL_HANDLE, &activeSwapchainImageId
    );

    // Recreate the swap chain if vkResult is suboptimal or out of data because we want the best possible vkResult.
    if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        XR_LOG_INFO(context->logger, "Swapchain out of date before presenting");
        xrRecreateSwapChain(context, models);
        return vkResult;
        ;
    }
    else if (vkResult == VK_SUBOPTIMAL_KHR)
    {
        XR_LOG_INFO(context->logger, "Swapchain suboptimal before presenting");
        xrRecreateSwapChain(context, models);
        return vkResult;
        ;
    }
    else if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    vkResult = vkResetFences(context->device, 1, &(context->inFlightFences[context->currentFrame]));
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    // Update the uniform buffer for current image.
    xrUpdateUniformBuffer(context, models, activeSwapchainImageId);

    VkSemaphore waitSemaphores[] = {context->imageAvailableSemaphores[context->currentFrame]};
    VkSemaphore signalSemaphores[] = {context->renderFinishedSemaphores[context->currentFrame]};
    VkPipelineStageFlags waitPipelineStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(sizeof(waitSemaphores) / sizeof(waitSemaphores[0]));
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitPipelineStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &(context->commandBuffers[activeSwapchainImageId]);
    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(sizeof(signalSemaphores) / sizeof(signalSemaphores[0]));
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResult = vkQueueSubmit(context->graphicsQueue, 1, &submitInfo, context->inFlightFences[context->currentFrame]);
    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    VkSwapchainKHR swapchains[] = {context->swapchain};

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(sizeof(signalSemaphores) / sizeof(signalSemaphores[0]));
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = static_cast<uint32_t>(sizeof(swapchains) / sizeof(swapchains[0]));
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &activeSwapchainImageId;
    presentInfo.pResults = nullptr;

    vkResult = vkQueuePresentKHR(context->presentQueue, &presentInfo);

    // Recreate the swap chain if vkResult is suboptimal or out of data because we want the best possible vkResult.
    if (vkResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        XR_LOG_INFO(context->logger, "Swapchain out of date after presenting");
        xrRecreateSwapChain(context, models);
        return vkResult;
    }
    else if (vkResult == VK_SUBOPTIMAL_KHR)
    {
        XR_LOG_INFO(context->logger, "Swapchain suboptimal after presenting");
        xrRecreateSwapChain(context, models);
        return vkResult;
    }
    else if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    xrWaitForIdle(context);
    return vkResult;
}

void xrUpdateUniformBuffer(XrContext *context, std::vector<XrModel *> models, uint32_t imageIndex)
{
    VkResult vkResult = VK_SUCCESS;

    for (size_t index = 0; index < models.size(); ++index)
    {
        XrModel *model = models[index];
        void *data = nullptr;
        vkResult = vkMapMemory(context->device, model->uniformBuffersMemory[imageIndex], 0, sizeof(XrUniformBufferObject), 0, &data);

        if (XR_IS_ERROR(vkResult))
        {
            XR_LOG_ERROR(context->logger, xrErrorName(vkResult), "Failed to update uniform buffer for model");
            continue;
        }

        memcpy(data, &model->ubo, sizeof(XrUniformBufferObject));
        vkUnmapMemory(context->device, model->uniformBuffersMemory[imageIndex]);
    }
}

// Debug methods

void xrPrintGpuProperties(XrContext *context, XrGpuDetails *gpuDetails, uint32_t currentGpuIndex, uint32_t totalGpuCount)
{
    if (!gpuDetails)
    {
        XR_LOG_INFO(context->logger, "No GPU properties to show!!!");
        return;
    }

    XR_LOG_INFO(context->logger, "---------- GPU Properties [%d/%d] [Rank: %d] ----------", currentGpuIndex, totalGpuCount, gpuDetails->rank);
    XR_LOG_INFO(context->logger, "Device Name\t\t: %s", gpuDetails->properties.deviceName);
    XR_LOG_INFO(context->logger, "Vendor Id\t\t: %d", gpuDetails->properties.vendorID);
    XR_LOG_INFO(context->logger, "Device Id\t\t: %d", gpuDetails->properties.deviceID);
    XR_LOG_INFO(context->logger, "Device Type\t\t: %d", gpuDetails->properties.deviceType);
    XR_LOG_INFO(context->logger, "API Version\t\t: %d", gpuDetails->properties.apiVersion);
    XR_LOG_INFO(context->logger, "Driver Version\t\t: %d", gpuDetails->properties.driverVersion);
    XR_LOG_UUID(context->logger, "Pipeline Cache UUID\t: ", gpuDetails->properties.pipelineCacheUUID);
    XR_LOG_INFO(context->logger, "Graphics Family Index\t\t: %d", gpuDetails->graphicsFamilyIndex);
    XR_LOG_INFO(context->logger, "Present Family Index\t\t: %d", gpuDetails->presentFamilyIndex);
    XR_LOG_INFO(context->logger, "Has Separate Present Queue\t: %d", gpuDetails->hasSeparatePresentQueue);
    XR_LOG_INFO(context->logger, "MSAA samples count: %d", gpuDetails->msaaSamples);
    XR_LOG_INFO(context->logger, "---------- GPU Properties End ----------");
}

void xrPrintInstanceLayerProperties(XrContext *context, std::vector<VkLayerProperties> properties)
{
#ifndef NDEBUG

    XR_LOG_INFO(context->logger, "---------- Instance Layer Properties ----------");

    for (VkLayerProperties &nextProperty : properties)
    {
        XR_LOG_INFO(context->logger, "Layer Name\t\t: %s", nextProperty.layerName);
        XR_LOG_INFO(context->logger, "Description\t\t: %s", nextProperty.description);
        XR_LOG_INFO(context->logger, "Spec Version\t\t: %d", nextProperty.specVersion);
        XR_LOG_INFO(context->logger, "Implementation Version\t: %d", nextProperty.implementationVersion);
        XR_LOG_INFO(context->logger, "------------------------------------------------------------");
    }

    XR_LOG_INFO(context->logger, "---------- Instance Layer Properties End [%d] ----------", properties.size());

#endif
}

void xrPrintDeviceLayerProperties(XrContext *context, std::vector<VkLayerProperties> properties)
{
#ifndef NDEBUG

    XR_LOG_INFO(context->logger, "---------- Device Layer Properties ----------");

    for (VkLayerProperties &nextProperty : properties)
    {
        XR_LOG_INFO(context->logger, "Layer Name\t\t: %s", nextProperty.layerName);
        XR_LOG_INFO(context->logger, "Description\t\t: %s", nextProperty.description);
        XR_LOG_INFO(context->logger, "Spec Version\t\t: %d", nextProperty.specVersion);
        XR_LOG_INFO(context->logger, "Implementation Version\t: %d", nextProperty.implementationVersion);
        XR_LOG_INFO(context->logger, "------------------------------------------------------------");
    }

    XR_LOG_INFO(context->logger, "---------- Device Layer Properties End [%d] ----------", properties.size());

#endif
}

void xrPrintSurfaceFormatsDetails(XrContext *context, std::vector<VkSurfaceFormatKHR> surfaceFormats)
{
#ifndef NDEBUG

    XR_LOG_INFO(context->logger, "---------- Surface Formats ----------");

    for (VkSurfaceFormatKHR &nextSurfaceFormat : surfaceFormats)
    {
        XR_LOG_INFO(context->logger, "format\t\t: %d", nextSurfaceFormat.format);
        XR_LOG_INFO(context->logger, "colorSpace\t: %d", nextSurfaceFormat.colorSpace);
        XR_LOG_INFO(context->logger, "------------------------------------------------------------");
    }

    XR_LOG_INFO(context->logger, "---------- Surface Formats Details End [%d] ----------", surfaceFormats.size());

#endif
}

void xrPrintSwapChainImageCount(XrContext *context, uint32_t minImageCount, uint32_t maxImageCount, uint32_t currentImageCount)
{
#ifndef NDEBUG

    XR_LOG_INFO(context->logger, "---------- Swapchain Image Count ----------");
    XR_LOG_INFO(context->logger, "Min\t: %d", minImageCount);
    XR_LOG_INFO(context->logger, "Max\t: %d", maxImageCount);
    XR_LOG_INFO(context->logger, "Current\t: %d", currentImageCount);
    XR_LOG_INFO(context->logger, "---------- Swapchain Image Count End ----------");

#endif
}
