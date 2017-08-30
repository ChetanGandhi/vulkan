#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#include <cstdlib>
#include <assert.h>
#include <iostream>
#include <sstream>
#include <set>
#include <chrono>
#include <unordered_map>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>
#include <tinyobj/tiny_obj_loader.h>

#include "buildParam.h"
#include "platform.h"
#include "renderer.h"
#include "utils.h"
#include "logger.h"

Renderer::Renderer(SurfaceSize surfaceSize)
{
    this->surfaceSize = surfaceSize;
    setupDebugLayer();
    setupLayersAndExtensions();
    initInstance();
    enableDebug(); // After initInstance as we need instance :P
}

Renderer::~Renderer()
{
    disableDebug();
    destroyInstance();
}

#if ENABLE_DEBUG

PFN_vkCreateDebugReportCallbackEXT _vkCreateDebugReportCallbackEXT = VK_NULL_HANDLE;
PFN_vkDestroyDebugReportCallbackEXT _vkDestroyDebugReportCallbackEXT = VK_NULL_HANDLE;

#endif

VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t sourceObject, size_t location, int32_t messageCode, const char *layerPrefix, const char *message, void *userData)
{
    #if ENABLE_DEBUG

    std::ostringstream stream;

    if(flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
    {
        stream<<"[INFO | ";
    }

    if(flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        stream<<"[WARNING | ";
    }

    if(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        stream<<"[ERROR | ";
    }

    if(flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
    {
        stream<<"[DEBUG | ";
    }

    stream<<layerPrefix<<"]: "<<message;
    LOG(stream.str());

    #if defined (_WIN32)

    if(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        MessageBox(NULL, stream.str().c_str(), TEXT("Vulkan Error"), MB_OK | MB_ICONERROR);
    }

    #endif // _WIN32

    #endif // ENABLE_DEBUG

    return false;
}

void Renderer::setupDebugLayer()
{
    #if ENABLE_DEBUG

    debugReportCallbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    debugReportCallbackInfo.pfnCallback = debugReportCallback;
    debugReportCallbackInfo.pNext = nullptr;
    debugReportCallbackInfo.pUserData = nullptr;
    debugReportCallbackInfo.flags = 0
    | (VK_DEBUG_REPORT_INFORMATION_BIT_EXT & ENABLE_DEBUG_REPORT_INFORMATION_BIT)
    | (VK_DEBUG_REPORT_WARNING_BIT_EXT & ENABLE_DEBUG_REPORT_WARNING_BIT)
    | (VK_DEBUG_REPORT_DEBUG_BIT_EXT & ENABLE_DEBUG_REPORT_DEBUG_BIT)
    | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
    | VK_DEBUG_REPORT_ERROR_BIT_EXT;

    instanceLayerList.push_back("VK_LAYER_LUNARG_standard_validation");
    // instanceLayerList.push_back("VK_LAYER_GOOGLE_threading");
    // instanceLayerList.push_back("VK_LAYER_LUNARG_image");
    // instanceLayerList.push_back("VK_LAYER_LUNARG_core_validation");
    // instanceLayerList.push_back("VK_LAYER_LUNARG_object_tracker");
    // instanceLayerList.push_back("VK_LAYER_LUNARG_parameter_validation");

    instanceExtensionList.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

    deviceLayerList.push_back("VK_LAYER_LUNARG_standard_validation");
    // deviceLayerList.push_back("VK_LAYER_LUNARG_threading");
    // deviceLayerList.push_back("VK_LAYER_LUNARG_image");
    // deviceLayerList.push_back("VK_LAYER_LUNARG_core_validation");
    // deviceLayerList.push_back("VK_LAYER_LUNARG_object_tracker");
    // deviceLayerList.push_back("VK_LAYER_LUNARG_parameter_validation");

    #endif // ENABLE_DEBUG
}

void Renderer::enableDebug()
{
    #if ENABLE_DEBUG

    _vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    _vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");

    if(_vkCreateDebugReportCallbackEXT == VK_NULL_HANDLE || _vkDestroyDebugReportCallbackEXT == VK_NULL_HANDLE)
    {
        assert(0 && "Vulkan Error: Cannot fetch debug functions");
        std::exit(EXIT_FAILURE);
    }

    _vkCreateDebugReportCallbackEXT(instance, &debugReportCallbackInfo, VK_NULL_HANDLE, &debugReport);

    #endif // ENABLE_DEBUG
}

void Renderer::disableDebug()
{
    #if ENABLE_DEBUG

    _vkDestroyDebugReportCallbackEXT(instance, debugReport, VK_NULL_HANDLE);
    debugReport = VK_NULL_HANDLE;

    #endif // ENABLE_DEBUG
}

void Renderer::setSurface(VkSurfaceKHR surface)
{
    this->surface = surface;
}

void Renderer::setSurfaceSize(SurfaceSize surfaceSize)
{
    this->surfaceSize = surfaceSize;
}

const VkInstance Renderer::getVulkanInstance() const
{
    return instance;
}

void Renderer::setupLayersAndExtensions()
{
    instanceExtensionList.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    instanceExtensionList.push_back(PLATFORM_SURFACE_EXTENSION_NAME);

    deviceExtensionList.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void Renderer::initInstance()
{
    VkApplicationInfo applicationInfo = {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext = nullptr;
    applicationInfo.apiVersion = VK_API_VERSION_1_0;
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pApplicationName = "Vulkan";
    applicationInfo.pEngineName = nullptr;
    applicationInfo.engineVersion = NULL;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = &debugReportCallbackInfo;
    instanceCreateInfo.flags = 0;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledLayerCount = instanceLayerList.size();
    instanceCreateInfo.ppEnabledLayerNames = instanceLayerList.data();
    instanceCreateInfo.enabledExtensionCount = instanceExtensionList.size();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensionList.data();

    VkResult result = vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &instance);
    CHECK_ERROR(result);
}

void Renderer::destroyInstance()
{
    vkDestroyInstance(instance, VK_NULL_HANDLE);
    instance = VK_NULL_HANDLE;
}

void Renderer::waitForIdle()
{
    vkQueueWaitIdle(graphicsQueue);

    if(queueFamilyIndices.hasSeparatePresentQueue)
    {
        vkQueueWaitIdle(presentQueue);
    }

    vkDeviceWaitIdle(device);
}

void Renderer::listAllPhysicalDevices(std::vector<GpuDetails> *gpuDetailsList)
{
    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(instance, &gpuCount, VK_NULL_HANDLE);

    if(gpuCount == 0)
    {
        return;
    }

    std::vector<VkPhysicalDevice> deviceList(gpuCount);
    vkEnumeratePhysicalDevices(instance, &gpuCount, deviceList.data());

    for(uint32_t counter = 0; counter < gpuCount; ++counter)
    {
        VkPhysicalDevice nextGpu = deviceList[counter];
        VkPhysicalDeviceProperties nextGpuProperties{};
        VkPhysicalDeviceMemoryProperties nextGpuMemoryProperties{};

        vkGetPhysicalDeviceProperties(nextGpu, &nextGpuProperties);
        vkGetPhysicalDeviceMemoryProperties(nextGpu, &nextGpuMemoryProperties);

        GpuDetails nextPhysicalDevice{};
        nextPhysicalDevice.gpu = nextGpu;
        nextPhysicalDevice.properties = nextGpuProperties;
        nextPhysicalDevice.memoryProperties = nextGpuMemoryProperties;
        gpuDetailsList->push_back(nextPhysicalDevice);
    }
}

bool Renderer::isDeviceSuitable(VkPhysicalDevice gpu)
{
    QueueFamilyIndices indices = {};

    bool suitableDeviceQueuesFound = findSuitableDeviceQueues(gpu, &indices);

    if(suitableDeviceQueuesFound)
    {
        queueFamilyIndices.graphicsFamilyIndex = indices.graphicsFamilyIndex;
        queueFamilyIndices.presentFamilyIndex = indices.presentFamilyIndex;
        queueFamilyIndices.hasSeparatePresentQueue = indices.hasSeparatePresentQueue;

        LOG("---------- Queue Family Indices ----------");
        LOGF("Graphics Family Index\t\t: %d", queueFamilyIndices.graphicsFamilyIndex);
        LOGF("Present Family Index\t\t: %d", queueFamilyIndices.presentFamilyIndex);
        LOGF("Has Separate Present Queue\t: %d", queueFamilyIndices.hasSeparatePresentQueue);
        LOG("---------- Queue Family Indices End ----------");
    }

    bool extensionSupported = checkDeviceExtensionSupport(gpu);
    bool swapchainSupported = true;

    if(extensionSupported)
    {
        SwapchainSupportDetails details = {};
        querySwapchainSupportDetails(gpu, &details);
        swapchainSupported = !details.surfaceFormats.empty() && !details.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures = {};
    vkGetPhysicalDeviceFeatures(gpu, &supportedFeatures);

    return suitableDeviceQueuesFound && extensionSupported && swapchainSupported && supportedFeatures.samplerAnisotropy;
}

bool Renderer::findSuitableDeviceQueues(VkPhysicalDevice gpu, QueueFamilyIndices *queueFamilyIndices)
{
    uint32_t familyCount = 0;
    uint32_t graphicsFamilyIndex = UINT32_MAX;
    uint32_t presentFamilyIndex = UINT32_MAX;

    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> familyPropertiesList(familyCount);
    std::vector<VkBool32> supportsPresentQueue(familyCount);

    for(uint32_t queueCounter = 0; queueCounter < familyCount; ++queueCounter)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, queueCounter, surface, &supportsPresentQueue.data()[queueCounter]);
    }

    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, familyPropertiesList.data());

    for(uint32_t queueCounter = 0; queueCounter < familyCount; ++queueCounter)
    {
        const VkQueueFamilyProperties nextFamilyProperties = familyPropertiesList[queueCounter];

        if(nextFamilyProperties.queueCount > 0 && nextFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsFamilyIndex = queueCounter;
        }

        if(nextFamilyProperties.queueCount > 0 && supportsPresentQueue.data()[queueCounter] == VK_TRUE)
        {
            graphicsFamilyIndex = queueCounter;
            presentFamilyIndex = queueCounter;
            break;
        }
    }

    if(presentFamilyIndex == UINT32_MAX)
    {
        for(uint32_t queueCounter = 0; queueCounter < familyCount; ++queueCounter)
        {
            if(supportsPresentQueue.data()[queueCounter] == VK_TRUE)
            {
                presentFamilyIndex = queueCounter;
                break;
            }
        }
    }

    if(graphicsFamilyIndex == UINT32_MAX || presentFamilyIndex == UINT32_MAX)
    {
        return false;
    }

    queueFamilyIndices->graphicsFamilyIndex = graphicsFamilyIndex;
    queueFamilyIndices->presentFamilyIndex = presentFamilyIndex;
    queueFamilyIndices->hasSeparatePresentQueue = (presentFamilyIndex != graphicsFamilyIndex);

    return true;
}

bool Renderer::checkDeviceExtensionSupport(VkPhysicalDevice gpu)
{
    uint32_t availableDeviceExtensionsCount = 0;

    VkResult result = vkEnumerateDeviceExtensionProperties(gpu, nullptr, &availableDeviceExtensionsCount, nullptr);

    CHECK_ERROR(result);

    if(availableDeviceExtensionsCount == 0)
    {
        return false;
    }

    std::vector<VkExtensionProperties> availableDeviceExtensions(availableDeviceExtensionsCount);
    result = vkEnumerateDeviceExtensionProperties(gpu, nullptr, &availableDeviceExtensionsCount, availableDeviceExtensions.data());

    CHECK_ERROR(result);

    std::set<std::string> requiredExtensions(deviceExtensionList.begin(), deviceExtensionList.end());

    for(const VkExtensionProperties &nextExtensionProperties : availableDeviceExtensions)
    {
        requiredExtensions.erase(nextExtensionProperties.extensionName);
    }

    return requiredExtensions.empty();
}

void Renderer::initDevice()
{
    {
        std::vector<GpuDetails> gpuDetailsList(0);
        listAllPhysicalDevices(&gpuDetailsList);

        uint32_t gpuCount = gpuDetailsList.size();
        uint32_t selectedGpuIndex = 0;

        LOGF("---------- Total GPU Found [%d]----------", gpuCount);

        for(uint32_t counter = 0; counter < gpuCount; ++counter)
        {
            GpuDetails nextGpuDetails = gpuDetailsList[counter];
            printGpuProperties(&nextGpuDetails.properties, counter + 1, gpuCount);
        }

        for(uint32_t counter = 0; counter < gpuCount; ++counter)
        {
            GpuDetails nextGpuDetails = gpuDetailsList[counter];

            if(isDeviceSuitable(nextGpuDetails.gpu))
            {
                gpuDetails = nextGpuDetails;
                selectedGpuIndex = counter;
                break;
            }
        }

        if(gpuDetails.gpu == VK_NULL_HANDLE)
        {
            assert(0 && "Vulkan Error: Queue family supporting graphics device not found.");
            std::exit(EXIT_FAILURE);
        }

        LOG("---------- Selected GPU Properties ----------");
        printGpuProperties(&gpuDetails.properties, (selectedGpuIndex + 1), gpuCount);
        LOG("---------- Selected GPU Properties End ----------");
    }

    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, VK_NULL_HANDLE);
        std::vector<VkLayerProperties> layerPropertiesList(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layerPropertiesList.data());
        printInstanceLayerProperties(layerPropertiesList);
    }

    {
        uint32_t layerCount = 0;
        vkEnumerateDeviceLayerProperties(gpuDetails.gpu, &layerCount, VK_NULL_HANDLE);
        std::vector<VkLayerProperties> layerPropertiesList(layerCount);
        vkEnumerateDeviceLayerProperties(gpuDetails.gpu, &layerCount, layerPropertiesList.data());
        printDeviceLayerProperties(layerPropertiesList);
    }
}

void Renderer::initLogicalDevice()
{
    std::vector<float> queuePriorities = {0.0f};
    std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos(0);

    VkDeviceQueueCreateInfo deviceGraphicQueueCreateInfo = {};
    deviceGraphicQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceGraphicQueueCreateInfo.pNext = nullptr;
    deviceGraphicQueueCreateInfo.flags = 0;
    deviceGraphicQueueCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamilyIndex;
    deviceGraphicQueueCreateInfo.queueCount = 1;
    deviceGraphicQueueCreateInfo.pQueuePriorities = queuePriorities.data();

    deviceQueueCreateInfos.push_back(deviceGraphicQueueCreateInfo);

    if(queueFamilyIndices.hasSeparatePresentQueue)
    {
        VkDeviceQueueCreateInfo devicePresentQueueCreateInfo {};
        devicePresentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        devicePresentQueueCreateInfo.pNext = nullptr;
        devicePresentQueueCreateInfo.flags = 0;
        devicePresentQueueCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamilyIndex;
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
    deviceCreateInfo.queueCreateInfoCount = deviceQueueCreateInfos.size();
    deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
    deviceCreateInfo.enabledLayerCount = deviceLayerList.size(); // Deprecated but still good for old API
    deviceCreateInfo.ppEnabledLayerNames = deviceLayerList.data(); // Deprecated but still good for old API
    deviceCreateInfo.enabledExtensionCount = deviceExtensionList.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionList.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    VkResult result = vkCreateDevice(gpuDetails.gpu, &deviceCreateInfo, VK_NULL_HANDLE, &device);
    CHECK_ERROR(result);

    // Create the graphic queue using graphicsFamilyIndex for given physical device.
    vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamilyIndex, 0, &graphicsQueue);

    if(!queueFamilyIndices.hasSeparatePresentQueue)
    {
        presentQueue = graphicsQueue;
    }
    else
    {
        vkGetDeviceQueue(device, queueFamilyIndices.presentFamilyIndex, 0, &presentQueue);
    }
}

void Renderer::destroyDevice()
{
    vkDestroyDevice(device, VK_NULL_HANDLE);
    device = VK_NULL_HANDLE;
}

void Renderer::querySwapchainSupportDetails(VkPhysicalDevice gpu, SwapchainSupportDetails *details)
{
    VkResult result = VK_SUCCESS;

    uint32_t formatCount = 0;
    uint32_t presentModeCount = 0;

    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &(details->surfaceCapabilities));
    CHECK_ERROR(result);

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, nullptr);
    CHECK_ERROR(result);

    details->surfaceFormats.resize(formatCount);

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, details->surfaceFormats.data());
    CHECK_ERROR(result);

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, nullptr);
    CHECK_ERROR(result);

    details->presentModes.resize(presentModeCount);

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, details->presentModes.data());
    CHECK_ERROR(result);
}

VkSurfaceFormatKHR Renderer::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &surfaceFormats)
{
    printSurfaceFormatsDetails(surfaceFormats);

    if(surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
    {

        VkSurfaceFormatKHR surfaceFormat = {};
        surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
        surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        return surfaceFormat;
    }

    for(const VkSurfaceFormatKHR &nextSurfaceFormat : surfaceFormats)
    {
        if(nextSurfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && nextSurfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return nextSurfaceFormat;
        }
    }

    return surfaceFormats[0];
}

VkPresentModeKHR Renderer::choosePresentMode(const std::vector<VkPresentModeKHR> presentModes)
{
    VkPresentModeKHR defaultPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    for(const VkPresentModeKHR &nextPresentMode : presentModes)
    {
        // If nextPresentMode is VK_PRESENT_MODE_MAILBOX_KHR then use this as this is the best.
        if(nextPresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return nextPresentMode;
        }

        // If VK_PRESENT_MODE_MAILBOX_KHR was not found then use VK_PRESENT_MODE_IMMEDIATE_KHR.
        if(nextPresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            defaultPresentMode = nextPresentMode;
        }
    }

    return defaultPresentMode;
}

void Renderer::chooseSurfaceExtent(VkSurfaceCapabilitiesKHR surfaceCapabilities, VkExtent2D *initialSurfaceExtent)
{
    if(surfaceCapabilities.currentExtent.width < UINT32_MAX)
    {
        initialSurfaceExtent->width = surfaceCapabilities.currentExtent.width;
        initialSurfaceExtent->height = surfaceCapabilities.currentExtent.height;
    }
    else
    {
        if(initialSurfaceExtent->width > surfaceCapabilities.maxImageExtent.width)
        {
            initialSurfaceExtent->width = surfaceCapabilities.maxImageExtent.width;
        }

        if(initialSurfaceExtent->width < surfaceCapabilities.minImageExtent.width)
        {
            initialSurfaceExtent->width = surfaceCapabilities.minImageExtent.width;
        }

        if(initialSurfaceExtent->height > surfaceCapabilities.maxImageExtent.height)
        {
            initialSurfaceExtent->height = surfaceCapabilities.maxImageExtent.height;
        }

        if(initialSurfaceExtent->height < surfaceCapabilities.minImageExtent.height)
        {
            initialSurfaceExtent->height = surfaceCapabilities.minImageExtent.height;
        }
    }
}

void Renderer::initSwapchain()
{
    VkExtent2D initialSurfaceExtent = {};
    initialSurfaceExtent.width = this->surfaceSize.width;
    initialSurfaceExtent.height = this->surfaceSize.height;

    querySwapchainSupportDetails(gpuDetails.gpu, &swapchainSupportDetails);

    if(!swapchainSupportDetails.surfaceFormats.size())
    {
        assert(0 && "Surface format missing.");
        std::exit(EXIT_FAILURE);
    }

    surfaceFormat = chooseSurfaceFormat(swapchainSupportDetails.surfaceFormats);
    chooseSurfaceExtent(swapchainSupportDetails.surfaceCapabilities, &initialSurfaceExtent);

    this->surfaceSize.width = initialSurfaceExtent.width;
    this->surfaceSize.height = initialSurfaceExtent.height;

    VkPresentModeKHR presentMode = choosePresentMode(swapchainSupportDetails.presentModes);

    // surfaceCapabilities.maxImageCount can be 0.
    // In this case the implementation supports unlimited amount of swap-chain images, limited by memory.
    // The amount of swap-chain images can also be fixed.
    if(swapchainImageCount < swapchainSupportDetails.surfaceCapabilities.minImageCount + 1)
    {
        swapchainImageCount = swapchainSupportDetails.surfaceCapabilities.minImageCount + 1;
    }

    if(swapchainImageCount > 0 && swapchainImageCount > swapchainSupportDetails.surfaceCapabilities.maxImageCount)
    {
        swapchainImageCount = swapchainSupportDetails.surfaceCapabilities.maxImageCount;
    }

    printSwapChainImageCount(swapchainSupportDetails.surfaceCapabilities.minImageCount, swapchainSupportDetails.surfaceCapabilities.maxImageCount, swapchainImageCount);

    {
        LOG("---------- Presentation Mode ----------");

        if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            LOGF("Mode: MAILBOX [%d]", presentMode);
        }
        else
        {
            LOGF("Mode: %d", presentMode);
        }

        LOG("---------- Presentation Mode End----------");
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = swapchainImageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent.width = surfaceSize.width;
    swapchainCreateInfo.imageExtent.height = surfaceSize.height;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.preTransform = swapchainSupportDetails.surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    if(queueFamilyIndices.hasSeparatePresentQueue)
    {
        std::vector<uint32_t> indices = {queueFamilyIndices.graphicsFamilyIndex, queueFamilyIndices.presentFamilyIndex};

        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = indices.size(); // Ignored if imageSharingMode is VK_SHARING_MODE_EXCLUSIVE
        swapchainCreateInfo.pQueueFamilyIndices = indices.data();
    }
    else
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0; // Ignored if imageSharingMode is VK_SHARING_MODE_EXCLUSIVE
        swapchainCreateInfo.pQueueFamilyIndices = nullptr; // Ignored if imageSharingMode is VK_SHARING_MODE_EXCLUSIVE
    }

    VkResult result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
    CHECK_ERROR(result);

    result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);
    CHECK_ERROR(result);

    swapchainImages.resize(swapchainImageCount);
    result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data());
    CHECK_ERROR(result);
}

void Renderer::destroySwapchain()
{
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void Renderer::initSwapchainImageViews()
{
    swapchainImageViews.resize(swapchainImageCount);

    for(uint32_t counter = 0; counter < swapchainImageCount; ++counter)
    {
        createImageView(swapchainImages[counter], surfaceFormat.format, swapchainImageViews[counter], VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void Renderer::destroySwapchainImageViews()
{
    for(VkImageView imageView : swapchainImageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }
}

VkShaderModule Renderer::createShaderModule(const std::vector<char> &code)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pNext = nullptr;
    shaderModuleCreateInfo.flags = 0;
    shaderModuleCreateInfo.codeSize = code.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);
    CHECK_ERROR(result);

    return shaderModule;
}

void Renderer::initGraphicsPipline()
{
    std::vector<char> vertexShaderCode;
    std::vector<char> fragmentShaderCode;

    if(!readFile("shaders/vert.spv", &vertexShaderCode))
    {
        assert(0 && "Cannot open vertex shader.");
    }

    if(!readFile("shaders/frag.spv", &fragmentShaderCode))
    {
        assert(0 && "Cannot open fragment shader.");
    }

    VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

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

    VkVertexInputBindingDescription vertexBindingDescription = Vertex::getBindingDescription();
    std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescription = Vertex::getAttributeDescription();

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.pNext = nullptr;
    vertexInputStateCreateInfo.flags = 0;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescription.size();
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
    viewport.width = (float) this->surfaceSize.width;
    viewport.height = (float) this->surfaceSize.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent.width = this->surfaceSize.width;
    scissor.extent.height = this->surfaceSize.height;

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
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
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
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
    | VK_COLOR_COMPONENT_G_BIT
    | VK_COLOR_COMPONENT_B_BIT
    | VK_COLOR_COMPONENT_A_BIT;

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

    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.pNext = nullptr;
    dynamicStateCreateInfo.flags = 0;
    dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = 0;

    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
    CHECK_ERROR(result);

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
    pipelineCreateInfo.pDynamicState = nullptr;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);
    CHECK_ERROR(result);

    vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(device, vertexShaderModule, nullptr);
}

void Renderer::destroyGraphicsPipline()
{
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

VkFormat Renderer::findSupportedFormat(VkPhysicalDevice gpu, const std::vector<VkFormat> &formatsToCheck, VkImageTiling imageTiling, VkFormatFeatureFlags formatFeatureFlags)
{
    for(VkFormat nextFormat : formatsToCheck)
    {
        VkFormatProperties formatProperties = {};
        vkGetPhysicalDeviceFormatProperties(gpu, nextFormat, &formatProperties);

        if(imageTiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & formatFeatureFlags) == formatFeatureFlags)
        {
            return nextFormat;
        }

        if(imageTiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & formatFeatureFlags) == formatFeatureFlags)
        {
            return nextFormat;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

VkFormat Renderer::findDepthFormat()
{
     std::vector<VkFormat> formatsToCheck = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    return findSupportedFormat(gpuDetails.gpu, formatsToCheck, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool Renderer::hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || false;
}

void Renderer::initDepthStencilImage()
{
    VkFormat depthStencilFormat = findDepthFormat();
    if(depthStencilFormat == VK_FORMAT_UNDEFINED)
    {
        assert(0 && "Depth stencil format not selected.");
    }

    bool stencilAvailable = hasStencilComponent(depthStencilFormat);
    createImage(surfaceSize.width, surfaceSize.height, depthStencilFormat,VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    createImageView(depthImage, depthStencilFormat, depthImageView, VK_IMAGE_ASPECT_DEPTH_BIT);
    transitionImageLayout(depthImage, depthStencilFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void Renderer::destoryDepthStencilImage()
{
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);
}

void Renderer::initRenderPass()
{
    VkAttachmentDescription colorAttachmentDescription = {};
    colorAttachmentDescription.flags = 0;
    colorAttachmentDescription.format = surfaceFormat.format;
    colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthStencilAttachmentDescription = {};
    depthStencilAttachmentDescription.flags = 0;
    depthStencilAttachmentDescription.format = findDepthFormat();
    depthStencilAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    depthStencilAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthStencilAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthStencilAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthStencilAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthStencilAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthStencilAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthStencilAttachmentReference = {};
    depthStencilAttachmentReference.attachment = 1;
    depthStencilAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.flags = 0;
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;
    subpassDescription.pResolveAttachments = nullptr;
    subpassDescription.pDepthStencilAttachment = &depthStencilAttachmentReference;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachmentDescription, depthStencilAttachmentDescription};

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
    renderPassCreateInfo.attachmentCount = attachments.size();
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    VkResult result = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass);
    CHECK_ERROR(result);
}

void Renderer::destroyRenderPass()
{
    vkDestroyRenderPass(device, renderPass, nullptr);
}

void Renderer::initDescriptorSetLayout()
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
    descriptorSetLayoutCreateInfo.bindingCount = layoutBindings.size();
    descriptorSetLayoutCreateInfo.pBindings = layoutBindings.data();

    VkResult result = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout);
    CHECK_ERROR(result);
}

void Renderer::destroyDescriptorSetLayout()
{
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}

void Renderer::initFrameBuffers()
{
    framebuffers.resize(swapchainImageCount);

    for(uint32_t swapchainImageCounter = 0; swapchainImageCounter < swapchainImageCount; ++swapchainImageCounter)
    {
        std::array<VkImageView, 2> attachments = {};
        attachments[0] = swapchainImageViews[swapchainImageCounter];
        attachments[1] = depthImageView;

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = nullptr;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = attachments.size();
        framebufferCreateInfo.pAttachments = attachments.data();
        framebufferCreateInfo.width = surfaceSize.width;
        framebufferCreateInfo.height = surfaceSize.height;
        framebufferCreateInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffers[swapchainImageCounter]);
        CHECK_ERROR(result);
    }
}

void Renderer::destroyFrameBuffers()
{
    for(VkFramebuffer nextFrameBuffer : framebuffers)
    {
        vkDestroyFramebuffer(device, nextFrameBuffer, nullptr);
    }
}

void Renderer::initCommandPool()
{
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = nullptr;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamilyIndex;

    VkResult result = vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);
    CHECK_ERROR(result);
}

void Renderer::destroyCommandPool()
{
    vkDestroyCommandPool(device, commandPool, nullptr);
}

void Renderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryPropertyFlags, VkImage &image, VkDeviceMemory &imageMemory)
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
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.usage = usage;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0;
    imageCreateInfo.pQueueFamilyIndices = nullptr;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult result = vkCreateImage(device, &imageCreateInfo, nullptr, &image);
    CHECK_ERROR(result);

    VkMemoryRequirements imageMemoryRequirements = {};
    vkGetImageMemoryRequirements(device, image, &imageMemoryRequirements);

    uint32_t memoryIndex = findMemoryTypeIndex(&(gpuDetails.memoryProperties), &imageMemoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo memoryAllocationInfo = {};
    memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocationInfo.pNext = nullptr;
    memoryAllocationInfo.allocationSize = imageMemoryRequirements.size;
    memoryAllocationInfo.memoryTypeIndex = memoryIndex;

    result = vkAllocateMemory(device, &memoryAllocationInfo, nullptr, &imageMemory);
    CHECK_ERROR(result);

    vkBindImageMemory(device, image, imageMemory, 0);
}

void Renderer::createImageView(VkImage image, VkFormat format, VkImageView &imageView, VkImageAspectFlags imageAspectFlags)
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
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView);
    CHECK_ERROR(result);
}

void Renderer::initTextureImage()
{
    int textureWidth = 0;
    int textureHeight = 0;
    int textureChannels = 0;

    stbi_uc *pixels = stbi_load(chaletTextureResourcePath.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
    VkDeviceSize size = textureWidth * textureHeight * 4;

    if(!pixels)
    {
        assert(0 && "Not able to load texture");
    }

    VkBuffer stagingImageBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingImageBufferMemory = VK_NULL_HANDLE;

    createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingImageBuffer, stagingImageBufferMemory);

    void *data = nullptr;
    vkMapMemory(device, stagingImageBufferMemory, 0, size, 0, &data);
    memcpy(data, pixels, size);
    vkUnmapMemory(device, stagingImageBufferMemory);
    stbi_image_free(pixels);

    createImage(static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingImageBuffer, textureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vkDestroyBuffer(device, stagingImageBuffer, nullptr);
    vkFreeMemory(device, stagingImageBufferMemory, nullptr);
}

void Renderer::destroyTextureImage()
{
    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);
}

void Renderer::initTextureImageView()
{
    createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, textureImageView, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Renderer::destroyTextureImageView()
{
    vkDestroyImageView(device, textureImageView, nullptr);
}

void Renderer::initTextureSampler()
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
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.anisotropyEnable = VK_TRUE;
    samplerCreateInfo.maxAnisotropy = 16;
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = 0.0f;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

    VkResult result = vkCreateSampler(device, &samplerCreateInfo, nullptr, &textureSampler);
    CHECK_ERROR(result);
}

void Renderer::destoryTextureSampler()
{
    vkDestroySampler(device, textureSampler, nullptr);
}

void Renderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProperties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
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

    VkResult result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer);
    CHECK_ERROR(result);

    VkMemoryRequirements bufferMemoryRequirements = {};
    vkGetBufferMemoryRequirements(device, buffer, &bufferMemoryRequirements);

    uint32_t memoryIndex = findMemoryTypeIndex(&(gpuDetails.memoryProperties), &bufferMemoryRequirements, memoryProperties);

    VkMemoryAllocateInfo memoryAllocationInfo = {};
    memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocationInfo.pNext = nullptr;
    memoryAllocationInfo.allocationSize = bufferMemoryRequirements.size;
    memoryAllocationInfo.memoryTypeIndex = memoryIndex;

    result = vkAllocateMemory(device, &memoryAllocationInfo, nullptr, &bufferMemory);
    CHECK_ERROR(result);

    result = vkBindBufferMemory(device, buffer, bufferMemory, 0);
    CHECK_ERROR(result);
}

void Renderer::beginOneTimeCommand(VkCommandBuffer &commandBuffer)
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VkResult result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
    CHECK_ERROR(result);

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
}

void Renderer::endOneTimeCommand(VkCommandBuffer &commandBuffer)
{
    VkResult result = vkEndCommandBuffer(commandBuffer);
    CHECK_ERROR(result);

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

    result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    CHECK_ERROR(result);

    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Renderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldImageLayout, VkImageLayout newImageLayout)
{
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    beginOneTimeCommand(commandBuffer);

    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = nullptr;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;

    if(newImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if(hasStencilComponent(format))
        {
            imageMemoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStageMask;
    VkPipelineStageFlags destinationStageMask;

    if(oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if(oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if(oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        assert(0 && "Unsupported layout transition!!!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStageMask, destinationStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

    endOneTimeCommand(commandBuffer);
}

void Renderer::copyBuffer(VkBuffer sourceBuffer, VkBuffer targetBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    beginOneTimeCommand(commandBuffer);

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    vkCmdCopyBuffer(commandBuffer, sourceBuffer, targetBuffer, 1, &copyRegion);

    endOneTimeCommand(commandBuffer);
}

void Renderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    beginOneTimeCommand(commandBuffer);

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

    endOneTimeCommand(commandBuffer);
}

void Renderer::loadModel()
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string error;

    bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &error, chaletModelResourcePath.c_str());

    if(!loaded) {
        LOGF("Model load error: %s", error.c_str());
        assert(0 && "Not able to load model.");
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

    for(const tinyobj::shape_t &nextShape : shapes)
    {
        for(const tinyobj::index_t &nextIndex : nextShape.mesh.indices)
        {
            Vertex nextVertex = {};
            nextVertex.position = {
                // the attrib.vertices array is an array of float values instead of something like glm::vec3,
                // so you need to multiply the index by 3 to create group of 3 values.
                attrib.vertices[3 * nextIndex.vertex_index + 0],
                attrib.vertices[3 * nextIndex.vertex_index + 1],
                attrib.vertices[3 * nextIndex.vertex_index + 2]
            };

            nextVertex.textureCoordinates = {
                // the attrib.texcoords array is an array of float values instead of something like glm::vec2,
                // so you need to multiply the index by 2 to create group of 2 values.
                attrib.texcoords[2 * nextIndex.texcoord_index + 0],
                1.0 - attrib.texcoords[2 * nextIndex.texcoord_index + 1]
            };

            nextVertex.color = {1.0f, 1.0f, 1.0f};

            if(uniqueVertices.count(nextVertex) == 0)
            {
                uniqueVertices[nextVertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(nextVertex);
            }

            vertexIndices.push_back(uniqueVertices[nextVertex]);
        }
    }
}

void Renderer::initVertexBuffer()
{
    VkDeviceSize size = sizeof(vertices[0]) * vertices.size();
    VkBufferUsageFlags stagingBufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags stagingMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    createBuffer(size, stagingBufferUsage, stagingMemoryProperties, stagingBuffer, stagingBufferMemory);

    void *stagingBufferData = nullptr;
    VkResult result = vkMapMemory(device, stagingBufferMemory, 0, size, 0, &stagingBufferData);
    CHECK_ERROR(result);

    memcpy(stagingBufferData, vertices.data(), (size_t)size);
    vkUnmapMemory(device, stagingBufferMemory);

    VkBufferUsageFlags vertexBufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VkMemoryPropertyFlags vertexMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    createBuffer(size, vertexBufferUsage, vertexMemoryProperties, vertexBuffer, vertexBufferMemory);
    copyBuffer(stagingBuffer, vertexBuffer, size);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Renderer::destroyVertexBuffer()
{
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
}

void Renderer::initIndexBuffer()
{
    VkDeviceSize size = sizeof(vertexIndices[0]) * vertexIndices.size();
    VkBufferUsageFlags stagingBufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags stagingMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    createBuffer(size, stagingBufferUsage, stagingMemoryProperties, stagingBuffer, stagingBufferMemory);

    void *stagingBufferData = nullptr;
    VkResult result = vkMapMemory(device, stagingBufferMemory, 0, size, 0, &stagingBufferData);
    CHECK_ERROR(result);

    memcpy(stagingBufferData, vertexIndices.data(), (size_t)size);
    vkUnmapMemory(device, stagingBufferMemory);

    VkBufferUsageFlags indexBufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    VkMemoryPropertyFlags indexMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    createBuffer(size, indexBufferUsage, indexMemoryProperties, indexBuffer, indexBufferMemory);
    copyBuffer(stagingBuffer, indexBuffer, size);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Renderer::destroyIndexBuffer()
{
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);
}

void Renderer::initUniformBuffer()
{
    VkDeviceSize size = sizeof(UniformBufferObject);
    VkBufferUsageFlags uniformBufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    VkMemoryPropertyFlags uniformMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    createBuffer(size, uniformBufferUsage, uniformMemoryProperties, uniformBuffer, uniformBufferMemory);
}

void Renderer::destroyUniformBuffer()
{
    vkDestroyBuffer(device, uniformBuffer, nullptr);
    vkFreeMemory(device, uniformBufferMemory, nullptr);
}

void Renderer::initDescriptorPool()
{
    VkDescriptorPoolSize uboPoolSize = {};
    uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboPoolSize.descriptorCount = 1;

    VkDescriptorPoolSize samplerPoolSize = {};
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = 1;

    std::array<VkDescriptorPoolSize, 2> poolSizes = {uboPoolSize, samplerPoolSize};

    VkDescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.pNext = nullptr;
    poolCreateInfo.flags = 0;
    // If you want to explicitly destroy the descriptorSet, then set this bit
    // else you will get runtime error while destroying the descriptorSet.
    // We are not going to used this for now.
    // poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolCreateInfo.maxSets = 1;
    poolCreateInfo.poolSizeCount = poolSizes.size();
    poolCreateInfo.pPoolSizes = poolSizes.data();

    VkResult result = vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &descriptorPool);
    CHECK_ERROR(result);
}

void Renderer::destroyDescriptorPool()
{
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void Renderer::initDescriptorSet()
{
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {descriptorSetLayout};

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts.data();

    VkResult result = vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet);
    CHECK_ERROR(result);

    VkDescriptorBufferInfo descriptorBufferInfo = {};
    descriptorBufferInfo.buffer = uniformBuffer;
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo descriptorImageInfo = {};
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptorImageInfo.imageView = textureImageView;
    descriptorImageInfo.sampler = textureSampler;

    VkWriteDescriptorSet uniformBudderDescriptorWrite = {};
    uniformBudderDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    uniformBudderDescriptorWrite.pNext = nullptr;
    uniformBudderDescriptorWrite.dstSet = descriptorSet;
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
    textureImageDescriptorWrite.dstSet = descriptorSet;
    textureImageDescriptorWrite.dstBinding = 1;
    textureImageDescriptorWrite.dstArrayElement = 0;
    textureImageDescriptorWrite.descriptorCount = 1;
    textureImageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureImageDescriptorWrite.pImageInfo = &descriptorImageInfo;
    textureImageDescriptorWrite.pBufferInfo = nullptr;
    textureImageDescriptorWrite.pTexelBufferView = nullptr;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites = {uniformBudderDescriptorWrite, textureImageDescriptorWrite};
    vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void Renderer::destroyDescriptorSet()
{
    // If you want to explicitly destroy the descriptorSet, then set
    // poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
    // bit in VkDescriptorPoolCreateInfo else you will get runtime error
    // while destroying the descriptorSet.
    // We are not going to used this for now.
    // vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet);
}

void Renderer::initCommandBuffers()
{
    commandBuffers.resize(framebuffers.size());

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = commandBuffers.size();

    VkResult result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers.data());
    CHECK_ERROR(result);

    for(uint32_t counter = 0; counter < commandBuffers.size(); ++counter)
    {
        VkCommandBufferBeginInfo commandBufferBeginInfo = {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.pNext = nullptr;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        commandBufferBeginInfo.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(commandBuffers[counter], &commandBufferBeginInfo);

        VkRect2D renderArea = {};
        renderArea.offset.x = 0;
        renderArea.offset.y = 0;
        renderArea.extent.width = surfaceSize.width;
        renderArea.extent.height = surfaceSize.height;

        std::array<VkClearValue, 2> clearValue = {};
        clearValue[0].color = {0.0f, 0.0f, 0.0f, 1.0f}; // {r, g, b, a}
        clearValue[1].depthStencil = {1.0f, 0}; // {depth, stencil}

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = framebuffers[counter];
        renderPassBeginInfo.renderArea = renderArea;
        renderPassBeginInfo.clearValueCount = clearValue.size();
        renderPassBeginInfo.pClearValues = clearValue.data();

        vkCmdBeginRenderPass(commandBuffers[counter], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffers[counter], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        std::vector<VkBuffer> vertexBuffers = {vertexBuffer};
        std::vector<VkDeviceSize> offsets = {0};
        vkCmdBindVertexBuffers(commandBuffers[counter], 0, 1, vertexBuffers.data(), offsets.data());
        vkCmdBindIndexBuffer(commandBuffers[counter], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffers[counter], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

        vkCmdDrawIndexed(commandBuffers[counter], vertexIndices.size(), 1, 0, 0, 0);
        vkCmdEndRenderPass(commandBuffers[counter]);

        VkResult result = vkEndCommandBuffer(commandBuffers[counter]);
        CHECK_ERROR(result);
    }
}

void Renderer::destroyCommandBuffers()
{
    vkFreeCommandBuffers(device, commandPool, commandBuffers.size(), commandBuffers.data());
}

void Renderer::initSynchronizations()
{
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    VkResult result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphore);
    CHECK_ERROR(result);

    result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphore);
    CHECK_ERROR(result);
}

void Renderer::destroySynchronizations()
{
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
}

void Renderer::recreateSwapChain()
{
    LOG("---------- Recreate SwapChain --------");
    vkDeviceWaitIdle(device);
    cleanupSwapChain();
    initSwapchain();
    initSwapchainImageViews();
    initRenderPass();
    initGraphicsPipline();
    initDepthStencilImage();
    initFrameBuffers();
    initCommandBuffers();
}

void Renderer::cleanupSwapChain()
{
    waitForIdle();
    destroyFrameBuffers();
    destoryDepthStencilImage();
    destroyCommandBuffers();
    destroyGraphicsPipline();
    destroyRenderPass();
    destroySwapchainImageViews();
    destroySwapchain();
}

void Renderer::render()
{
    uint32_t activeSwapchainImageId = UINT32_MAX;

    VkResult result = vkQueueWaitIdle(graphicsQueue);
    CHECK_ERROR(result);

    result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &activeSwapchainImageId);

    // If result is VK_ERROR_OUT_OF_DATE_KHR than just recreate swap chain
    // as current swap chain cannot be used with current surface.
    if(result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapChain();
        return;
    }

    CHECK_ERROR(result);

    std::vector<VkSemaphore> waitSemaphores = {imageAvailableSemaphore};
    std::vector<VkSemaphore> signalSemaphores = {renderFinishedSemaphore};
    std::vector<VkPipelineStageFlags> waitPipelineStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitPipelineStages.data();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[activeSwapchainImageId];
    submitInfo.signalSemaphoreCount = signalSemaphores.size();
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    CHECK_ERROR(result);

    std::vector<VkSwapchainKHR> swapchains = {swapchain};

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = signalSemaphores.size();
    presentInfo.pWaitSemaphores = signalSemaphores.data();
    presentInfo.swapchainCount = swapchains.size();
    presentInfo.pSwapchains = swapchains.data();
    presentInfo.pImageIndices = &activeSwapchainImageId;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    // Recreate the swap chain if result is suboptimal,
    // because we want the best possible result.
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        recreateSwapChain();
    }
    else
    {
        CHECK_ERROR(result);
    }

    vkQueueWaitIdle(presentQueue);
}

void Renderer::updateUniformBuffer()
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

    // To push object deep into screen, modify the eye matrix to have more positive (greater) value at z-axis.
    UniformBufferObject ubo = {};
    ubo.model = glm::rotate(glm::mat4(), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.projection = glm::perspective(glm::radians(45.0f), (float)surfaceSize.width / (float)surfaceSize.height, 0.1f, 10.0f);

    //The GLM is designed for OpenGL, where the Y coordinate of the clip coordinate is inverted.
    // If we do not fix this then the image will be rendered upside-down.
    // The easy way to fix this is to flip the sign on the scaling factor of Y axis
    // in the projection matrix.
    ubo.projection[1][1] *= -1;

    void *data = nullptr;
    vkMapMemory(device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, uniformBufferMemory);
}

// Debug methods

void Renderer::printGpuProperties(VkPhysicalDeviceProperties *properties, uint32_t currentGpuIndex, uint32_t totalGpuCount)
{
    if(!properties)
    {
        LOG("No GPU properties to show!!!");
        return;
    }

    LOGF("---------- GPU Properties [%d/%d]----------", currentGpuIndex, totalGpuCount);
    LOGF("Device Name\t\t: %s", properties->deviceName);
    LOGF("Vendor Id\t\t: %d", properties->vendorID);
    LOGF("Device Id\t\t: %d", properties->deviceID);
    LOGF("Device Type\t\t: %d", properties->deviceType);
    LOGF("API Version\t\t: %d", properties->apiVersion);
    LOGF("Driver Version\t\t: %d", properties->driverVersion);
    LOG_UUID("Pipeline Cache UUID\t: ", properties->pipelineCacheUUID);
    LOGF("---------- GPU Properties End ----------");
}

void Renderer::printInstanceLayerProperties(std::vector<VkLayerProperties> properties)
{
    #if ENABLE_DEBUG

    LOG("---------- Instance Layer Properties ----------");

    for(VkLayerProperties &nextProperty : properties)
    {
        LOGF("Layer Name\t\t: %s", nextProperty.layerName);
        LOGF("Description\t\t: %s", nextProperty.description);
        LOGF("Spec Version\t\t: %d", nextProperty.specVersion);
        LOGF("Implementation Version\t: %d", nextProperty.implementationVersion);
        LOG("------------------------------------------------------------");
    }

    LOGF("---------- Instance Layer Properties End [%d] ----------", properties.size());

    #endif // ENABLE_DEBUG
}

void Renderer::printDeviceLayerProperties(std::vector<VkLayerProperties> properties)
{
    #if ENABLE_DEBUG

    LOG("---------- Device Layer Properties ----------");

    for(VkLayerProperties &nextProperty : properties)
    {
        LOGF("Layer Name\t\t: %s", nextProperty.layerName);
        LOGF("Description\t\t: %s", nextProperty.description);
        LOGF("Spec Version\t\t: %d", nextProperty.specVersion);
        LOGF("Implementation Version\t: %d", nextProperty.implementationVersion);
        LOG("------------------------------------------------------------");
    }

    LOGF("---------- Device Layer Properties End [%d] ----------", properties.size());

    #endif // ENABLE_DEBUG
}

void Renderer::printSurfaceFormatsDetails(std::vector<VkSurfaceFormatKHR> surfaceFormats)
{
    #if ENABLE_DEBUG

    LOG("---------- Surface Formats ----------");

    for(VkSurfaceFormatKHR &nextSurfaceFormat : surfaceFormats)
    {
        LOGF("format\t\t: %d", nextSurfaceFormat.format);
        LOGF("colorSpace\t: %d", nextSurfaceFormat.colorSpace);
        LOG("------------------------------------------------------------");
    }

    LOGF("---------- Surface Formats Details End [%d] ----------", surfaceFormats.size());

    #endif // ENABLE_DEBUG
}

void Renderer::printSwapChainImageCount(uint32_t minImageCount, uint32_t maxImageCount, uint32_t currentImageCount)
{
    #if ENABLE_DEBUG

    LOG("---------- Swapchain Image Count ----------");
    LOGF("Min\t: %d", minImageCount);
    LOGF("Max\t: %d", maxImageCount);
    LOGF("Current\t: %d", currentImageCount);
    LOG("---------- Swapchain Image Count End ----------");

    #endif // ENABLE_DEBUG
}
