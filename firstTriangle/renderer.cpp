#pragma once

#include "buildParam.h"
#include "platform.h"
#include "renderer.h"
#include "utils.h"

#include <cstdlib>
#include <assert.h>
#include <iostream>
#include <sstream>
#include <set>

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

    stream<<layerPrefix<<"]: "<<message<<"\n";
    std::cout<<stream.str();

    #ifdef _WIN32

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
    VkApplicationInfo applicationInfo {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext = nullptr;
    applicationInfo.apiVersion = VK_API_VERSION_1_0;
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pApplicationName = "Vulkan";
    applicationInfo.pEngineName = nullptr;
    applicationInfo.engineVersion = NULL;

    VkInstanceCreateInfo instanceCreateInfo {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = &debugReportCallbackInfo;
    instanceCreateInfo.flags = 0;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledLayerCount = instanceLayerList.size();
    instanceCreateInfo.ppEnabledLayerNames = instanceLayerList.data();
    instanceCreateInfo.enabledExtensionCount = instanceExtensionList.size();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensionList.data();

    VkResult result = vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &instance);
    checkError(result, __FILE__, __LINE__);
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
        VkPhysicalDeviceProperties nextGpuProperties {};
        VkPhysicalDeviceMemoryProperties nextGpuMemoryProperties {};

        vkGetPhysicalDeviceProperties(nextGpu, &nextGpuProperties);
        vkGetPhysicalDeviceMemoryProperties(nextGpu, &nextGpuMemoryProperties);

        GpuDetails nextPhysicalDevice {};
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

        std::cout<<"\n---------- Queue Family Indices ----------\n";
        std::cout<<"\nGraphics Family Index\t\t: "<<queueFamilyIndices.graphicsFamilyIndex;
        std::cout<<"\nPresent Family Index\t\t: "<<queueFamilyIndices.presentFamilyIndex;
        std::cout<<"\nHas Separate Present Queue\t: "<<queueFamilyIndices.hasSeparatePresentQueue<<"\n";
        std::cout<<"\n---------- Queue Family Indices End ----------\n";
    }

    bool extensionSupported = checkDeviceExtensionSupport(gpu);
    bool swapchainSupported = true;

    if(extensionSupported)
    {
        SwapchainSupportDetails details = {};
        querySwapchainSupportDetails(gpu, &details);
        swapchainSupported = !details.surfaceFormats.empty() && !details.presentModes.empty();
    }

    return (suitableDeviceQueuesFound && extensionSupported && swapchainSupported);
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

    checkError(result, __FILE__, __LINE__);

    if(availableDeviceExtensionsCount == 0)
    {
        return false;
    }

    std::vector<VkExtensionProperties> availableDeviceExtensions(availableDeviceExtensionsCount);
    result = vkEnumerateDeviceExtensionProperties(gpu, nullptr, &availableDeviceExtensionsCount, availableDeviceExtensions.data());

    checkError(result, __FILE__, __LINE__);

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

        std::cout<<"\n---------- Total GPU Found ["<<gpuCount<<"]----------\n";

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

        std::cout<<"\n---------- Selected GPU Properties ----------\n";
        printGpuProperties(&gpuDetails.properties, (selectedGpuIndex + 1), gpuCount);
        std::cout<<"\n---------- Selected GPU Properties End ----------\n";
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

    // This is not needed right now.
    // This have many VkBool32 properties, leave it to VK_FALSE right now.
    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo deviceCreateInfo {};
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
    checkError(result, __FILE__, __LINE__);

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
    checkError(result, __FILE__, __LINE__);

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, nullptr);
    checkError(result, __FILE__, __LINE__);

    details->surfaceFormats.resize(formatCount);

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, details->surfaceFormats.data());
    checkError(result, __FILE__, __LINE__);

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, nullptr);
    checkError(result, __FILE__, __LINE__);

    details->presentModes.resize(presentModeCount);

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, details->presentModes.data());
    checkError(result, __FILE__, __LINE__);
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
        #if ENABLE_DEBUG

        std::cout<<"\n---------- Presentation Mode ----------\n";
        if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            std::cout<<"\nMode: MAILBOX ["<<presentMode<<"]\n";
        }
        else
        {
            std::cout<<"\nMode: "<<presentMode<<"\n";
        }
        std::cout<<"\n---------- Presentation Mode End----------\n\n";

        #endif // ENABLE_DEBUG
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo {};
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
    checkError(result, __FILE__, __LINE__);

    result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);
    checkError(result, __FILE__, __LINE__);

    swapchainImages.resize(swapchainImageCount);
    result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data());
    checkError(result, __FILE__, __LINE__);
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
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;
        imageViewCreateInfo.flags = 0;
        imageViewCreateInfo.image = swapchainImages[counter];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = surfaceFormat.format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[counter]);
        checkError(result, __FILE__, __LINE__);
    }
}

void Renderer::destroySwapchainImageViews()
{
    for(VkImageView imageView: swapchainImageViews)
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
    checkError(result, __FILE__, __LINE__);

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
    std::array<VkVertexInputAttributeDescription, 2> vertexAttributeDescription = Vertex::getAttributeDescription();

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
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
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

    // Not required for now.
    // VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {}

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

    std::vector<VkDynamicState> dynamicStates { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };

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
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = 0;

    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
    checkError(result, __FILE__, __LINE__);

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
    pipelineCreateInfo.pDepthStencilState = nullptr;
    pipelineCreateInfo.pColorBlendState = &colorBlendingStateCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);
    checkError(result, __FILE__, __LINE__);

    vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(device, vertexShaderModule, nullptr);
}

void Renderer::destroyGraphicsPipline()
{
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}


// void Renderer::initDepthStencilImage()
// {
//     {
//         std::vector<VkFormat> formatsToTry
//         {
//             VK_FORMAT_D32_SFLOAT_S8_UINT,
//             VK_FORMAT_D24_UNORM_S8_UINT,
//             VK_FORMAT_D16_UNORM_S8_UINT,
//             VK_FORMAT_D32_SFLOAT,
//             VK_FORMAT_D16_UNORM
//         };

//         for(VkFormat nextFormat : formatsToTry)
//         {
//             VkFormatProperties formatProperties {};
//             vkGetPhysicalDeviceFormatProperties(getVulkanPhysicalDevice(), nextFormat, &formatProperties);

//             if(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
//             {
//                 depthStencilFormat = nextFormat;
//                 break;
//             }
//         }

//         if(depthStencilFormat == VK_FORMAT_UNDEFINED)
//         {
//             assert(0 && "Depth stencil format not selected.");
//             std::exit(EXIT_FAILURE);
//         }

//         stencilAvailable = (depthStencilFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || depthStencilFormat == VK_FORMAT_D24_UNORM_S8_UINT || depthStencilFormat == VK_FORMAT_D16_UNORM_S8_UINT || depthStencilFormat == VK_FORMAT_S8_UINT) || false;
//     }

//     VkImageCreateInfo imageCreateInfo {};
//     imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
//     imageCreateInfo.pNext = nullptr;
//     imageCreateInfo.flags = 0;
//     imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
//     imageCreateInfo.format = depthStencilFormat;
//     imageCreateInfo.extent.width = surfaceSize.width;
//     imageCreateInfo.extent.height = surfaceSize.height;
//     imageCreateInfo.extent.depth = 1;
//     imageCreateInfo.mipLevels = 1;
//     imageCreateInfo.arrayLayers = 1;
//     imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
//     imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
//     imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
//     imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//     imageCreateInfo.queueFamilyIndexCount = VK_QUEUE_FAMILY_IGNORED;
//     imageCreateInfo.pQueueFamilyIndices = nullptr;
//     imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

//     VkResult result = vkCreateImage(getVulkanDevice(), &imageCreateInfo, nullptr, &depthStencil.image);

//     checkError(result, __FILE__, __LINE__);

//     VkMemoryRequirements imageMemoryRequirements {};
//     vkGetImageMemoryRequirements(getVulkanDevice(), depthStencil.image, &imageMemoryRequirements);

//     VkPhysicalDeviceMemoryProperties gpuMemoryProperties = getVulkanPhysicalDeviceMemoryProperties();
//     VkMemoryPropertyFlagBits requiredMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

//     uint32_t memoryIndex = findMemoryTypeIndex(&gpuMemoryProperties, &imageMemoryRequirements, requiredMemoryProperties);

//     VkMemoryAllocateInfo memoryAllocationInfo {};
//     memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//     memoryAllocationInfo.pNext = nullptr;
//     memoryAllocationInfo.allocationSize = imageMemoryRequirements.size;
//     memoryAllocationInfo.memoryTypeIndex = memoryIndex;

//     result = vkAllocateMemory(getVulkanDevice(), &memoryAllocationInfo, nullptr, &depthStencil.imageMemory);

//     checkError(result, __FILE__, __LINE__);

//     result = vkBindImageMemory(getVulkanDevice(), depthStencil.image, depthStencil.imageMemory, 0);

//     checkError(result, __FILE__, __LINE__);

//     VkImageViewCreateInfo imageViewCreateInfo {};
//     imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//     imageViewCreateInfo.pNext = nullptr;
//     imageViewCreateInfo.flags = 0;
//     imageViewCreateInfo.image = depthStencil.image;
//     imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//     imageViewCreateInfo.format = depthStencilFormat;
//     imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
//     imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
//     imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
//     imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
//     imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (stencilAvailable ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
//     imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
//     imageViewCreateInfo.subresourceRange.levelCount = 1;
//     imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
//     imageViewCreateInfo.subresourceRange.layerCount = 1;

//     result = vkCreateImageView(getVulkanDevice(), &imageViewCreateInfo, nullptr, &depthStencil.imageView);

//     checkError(result, __FILE__, __LINE__);
// }

// void Renderer::destoryDepthStencilImage()
// {
//     vkDestroyImageView(getVulkanDevice(), depthStencil.imageView, nullptr);
//     vkFreeMemory(getVulkanDevice(), depthStencil.imageMemory, nullptr);
//     vkDestroyImage(getVulkanDevice(), depthStencil.image, nullptr);
// }

void Renderer::initRenderPass()
{
    std::array<VkAttachmentDescription, 1> attachments {};
    // Depth and stencil attachment.
    // attachments[0].flags = 0;
    // attachments[0].format = depthStencilFormat;
    // attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    // attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    // attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    // attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Color attachment.
    attachments[0].flags = 0;
    attachments[0].format = surfaceFormat.format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // VkAttachmentReference subpassDepthStencilAttachment {};
    // subpassDepthStencilAttachment.attachment = 0;
    // subpassDepthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentReference, 1> subpassColourAttachments {};
    subpassColourAttachments[0].attachment = 0;
    subpassColourAttachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    std::array<VkSubpassDescription, 1> subpasses {};
    subpasses[0].flags = 0;
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].inputAttachmentCount = 0;
    subpasses[0].pInputAttachments = nullptr;
    subpasses[0].colorAttachmentCount = subpassColourAttachments.size();
    subpasses[0].pColorAttachments = subpassColourAttachments.data();
    subpasses[0].pResolveAttachments = nullptr;
    subpasses[0].pDepthStencilAttachment = nullptr; // &subpassDepthStencilAttachment;
    subpasses[0].preserveAttachmentCount = 0;
    subpasses[0].pPreserveAttachments = nullptr;

    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = attachments.size();
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = subpasses.size();
    renderPassCreateInfo.pSubpasses = subpasses.data();
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    VkResult result = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass);
    checkError(result, __FILE__, __LINE__);
}

void Renderer::destroyRenderPass()
{
    vkDestroyRenderPass(device, renderPass, nullptr);
}

void Renderer::initFrameBuffers()
{
    framebuffers.resize(swapchainImageCount);

    for(uint32_t swapchainImageCounter = 0; swapchainImageCounter < swapchainImageCount; ++swapchainImageCounter)
    {
        std::array<VkImageView, 1> attachments {};
        // attachments[0] = depthStencil.imageView;
        attachments[0] = swapchainImageViews[swapchainImageCounter];

        VkFramebufferCreateInfo framebufferCreateInfo {};
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
        checkError(result, __FILE__, __LINE__);
    }
}

void Renderer::destroyFrameBuffers()
{
    for(VkFramebuffer nextFrameuffer :framebuffers)
    {
        vkDestroyFramebuffer(device, nextFrameuffer, nullptr);
    }
}

void Renderer::initCommandPool()
{
    VkCommandPoolCreateInfo commandPoolCreateInfo {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = nullptr;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamilyIndex;

    VkResult result = vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);
    checkError(result, __FILE__, __LINE__);
}

void Renderer::destroyCommandPool()
{
    vkDestroyCommandPool(device, commandPool, nullptr);
}

void Renderer::initVertexBuffer()
{
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.pNext = nullptr;
    bufferCreateInfo.flags = 0;
    bufferCreateInfo.size = sizeof(vertices[0]) * vertices.size();
    bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = 0;
    bufferCreateInfo.pQueueFamilyIndices = nullptr; // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT

    VkResult result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &vertexBuffer);
    checkError(result, __FILE__, __LINE__);

    VkMemoryRequirements bufferMemoryRequirements = {};
    vkGetBufferMemoryRequirements(device, vertexBuffer, &bufferMemoryRequirements);

    VkMemoryPropertyFlags requiredMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    uint32_t memoryIndex = findMemoryTypeIndex(&(gpuDetails.memoryProperties), &bufferMemoryRequirements, requiredMemoryProperties);

    VkMemoryAllocateInfo memoryAllocationInfo {};
    memoryAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocationInfo.pNext = nullptr;
    memoryAllocationInfo.allocationSize = bufferMemoryRequirements.size;
    memoryAllocationInfo.memoryTypeIndex = memoryIndex;

    result = vkAllocateMemory(device, &memoryAllocationInfo, nullptr, &vertexBufferMemory);
    checkError(result, __FILE__, __LINE__);

    result = vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);
    checkError(result, __FILE__, __LINE__);

    void *data = nullptr;
    result = vkMapMemory(device, vertexBufferMemory, 0, bufferCreateInfo.size, 0, &data);
    checkError(result, __FILE__, __LINE__);

    memcpy(data, vertices.data(), (size_t) bufferCreateInfo.size);
    vkUnmapMemory(device, vertexBufferMemory);
}

void Renderer::destroyVertexBuffer()
{
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
}

void Renderer::initCommandBuffers()
{
    commandBuffers.resize(framebuffers.size());

    VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = commandBuffers.size();

    VkResult result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers.data());
    checkError(result, __FILE__, __LINE__);

    for(uint32_t counter = 0; counter < commandBuffers.size(); ++counter)
    {
        VkCommandBufferBeginInfo commandBufferBeginInfo {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.pNext = nullptr;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        commandBufferBeginInfo.pInheritanceInfo = nullptr;

        vkBeginCommandBuffer(commandBuffers[counter], &commandBufferBeginInfo);

        VkRect2D renderArea {};
        renderArea.offset.x = 0;
        renderArea.offset.y = 0;
        renderArea.extent.width = surfaceSize.width;
        renderArea.extent.height = surfaceSize.height;

        std::array<VkClearValue, 1> clearValue {};
        // clearValue[0].depthStencil.depth = 0.0f;
        // clearValue[0].depthStencil.stencil = 0;
        clearValue[0].color.float32[0] = 0.0f;
        clearValue[0].color.float32[1] = 0.0f;
        clearValue[0].color.float32[2] = 0.0f;
        clearValue[0].color.float32[3] = 1.0f;

        VkRenderPassBeginInfo renderPassBeginInfo {};
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

        vkCmdDraw(commandBuffers[counter], vertices.size(), 1, 0, 0);
        vkCmdEndRenderPass(commandBuffers[counter]);

        VkResult result = vkEndCommandBuffer(commandBuffers[counter]);
        checkError(result, __FILE__, __LINE__);
    }
}

void Renderer::destroyCommandBuffers()
{
    vkFreeCommandBuffers(device, commandPool, commandBuffers.size(), commandBuffers.data());
}

void Renderer::initSynchronizations()
{
    VkSemaphoreCreateInfo semaphoreCreateInfo {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    VkResult result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphore);
    checkError(result, __FILE__, __LINE__);

    result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphore);
    checkError(result, __FILE__, __LINE__);
}

void Renderer::destroySynchronizations()
{
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
}

void Renderer::recreateSwapChain()
{
    vkDeviceWaitIdle(device);
    cleanupSwapChain();
    initSwapchain();
    initSwapchainImageViews();
    // initDepthStencilImage();
    initRenderPass();
    initGraphicsPipline();
    initFrameBuffers();
    initCommandBuffers();
}

void Renderer::cleanupSwapChain()
{
    waitForIdle();
    destroyCommandBuffers();
    destroyFrameBuffers();
    destroyGraphicsPipline();
    destroyRenderPass();
    // destoryDepthStencilImage();
    destroySwapchainImageViews();
    destroySwapchain();
}

void Renderer::render()
{
    uint32_t activeSwapchainImageId = UINT32_MAX;

    VkResult result = vkQueueWaitIdle(graphicsQueue);
    checkError(result, __FILE__, __LINE__);

    result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &activeSwapchainImageId);

    // If result is VK_ERROR_OUT_OF_DATE_KHR than just recreate swap chain
    // as current swap chain cannot be used with current surface.
    if(result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapChain();
        return;
    }

    checkError(result, __FILE__, __LINE__);

    std::vector<VkSemaphore> waitSemaphores = {imageAvailableSemaphore};
    std::vector<VkSemaphore> signalSemaphores = {renderFinishedSemaphore};
    std::vector<VkPipelineStageFlags> waitPipelineStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo {};
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
    checkError(result, __FILE__, __LINE__);

    std::vector<VkSwapchainKHR> swapchains = {swapchain};

    VkPresentInfoKHR presentInfo {};
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
        checkError(result, __FILE__, __LINE__);
    }

    vkQueueWaitIdle(presentQueue);
}

// Debug methods

void Renderer::printGpuProperties(VkPhysicalDeviceProperties *properties, uint32_t currentGpuIndex, uint32_t totalGpuCount)
{
    if(!properties)
    {
        std::cout<<"No GPU properties to show!!!"<<std::endl;
        return;
    }

    std::cout<<"\n---------- GPU Properties ["<<currentGpuIndex<<"/"<<totalGpuCount<<"]----------\n";
    std::cout<<"Device Name\t\t: "<<properties->deviceName<<"\n";
    std::cout<<"Vendor Id\t\t: "<<properties->vendorID<<"\n";
    std::cout<<"Device Id\t\t: "<<properties->deviceID<<"\n";
    std::cout<<"Device Type\t\t: "<<properties->deviceType<<"\n";
    std::cout<<"API Version\t\t: "<<properties->apiVersion<<"\n";
    std::cout<<"Driver Version\t\t: "<<properties->driverVersion<<"\n";
    std::cout<<"Pipeline Cache UUID\t: "<<properties->pipelineCacheUUID<<"\n";
    std::cout<<"\n---------- GPU Properties End ----------\n";
}

void Renderer::printInstanceLayerProperties(std::vector<VkLayerProperties> properties)
{
    std::cout<<"\n---------- Instance Layer Properties ----------\n";
    for(VkLayerProperties &nextProperty : properties)
    {
        std::cout<<"Layer Name\t\t: "<<nextProperty.layerName<<"\n";
        std::cout<<"Description\t\t: "<<nextProperty.description<<"\n";
        std::cout<<"Spec Version\t\t: "<<nextProperty.specVersion<<"\n";
        std::cout<<"Implementation Version\t: "<<nextProperty.implementationVersion<<"\n";
        std::cout<<"------------------------------------------------------------\n";
    }

    std::cout<<"\n---------- Instance Layer Properties End ["<<properties.size()<<"] ----------\n";
}

void Renderer::printDeviceLayerProperties(std::vector<VkLayerProperties> properties)
{
    std::cout<<"\n---------- Device Layer Properties ----------\n";
    for(VkLayerProperties &nextProperty : properties)
    {
        std::cout<<"Layer Name\t\t: "<<nextProperty.layerName<<"\n";
        std::cout<<"Description\t\t: "<<nextProperty.description<<"\n";
        std::cout<<"Spec Version\t\t: "<<nextProperty.specVersion<<"\n";
        std::cout<<"Implementation Version\t: "<<nextProperty.implementationVersion<<"\n";
        std::cout<<"------------------------------------------------------------\n";
    }

    std::cout<<"\n---------- Device Layer Properties End ["<<properties.size()<<"] ----------\n\n";
}

void Renderer::printSurfaceFormatsDetails(std::vector<VkSurfaceFormatKHR> surfaceFormats)
{
    #if ENABLE_DEBUG

    std::cout<<"\n---------- Surface Formats ----------\n";
    for(VkSurfaceFormatKHR &nextSurfaceFormat : surfaceFormats)
    {
        std::cout<<"format\t\t: "<<nextSurfaceFormat.format<<"\n";
        std::cout<<"colorSpace\t: "<<nextSurfaceFormat.colorSpace<<"\n";
        std::cout<<"------------------------------------------------------------\n";
    }

    std::cout<<"\n---------- Surface Formats Details End ["<<surfaceFormats.size()<<"] ----------\n\n";

    #endif // ENABLE_DEBUG
}

void Renderer::printSwapChainImageCount(uint32_t minImageCount, uint32_t maxImageCount, uint32_t currentImageCount)
{
    #if ENABLE_DEBUG

    std::cout<<"\n---------- Swapchain Image Count ----------\n";
    std::cout<<"Min\t: "<<minImageCount<<"\n";
    std::cout<<"Max\t: "<<maxImageCount<<"\n";
    std::cout<<"Current\t: "<<currentImageCount<<"\n";

    std::cout<<"\n---------- Swapchain Image Count End ----------\n\n";

    #endif // ENABLE_DEBUG
}
