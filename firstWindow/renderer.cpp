#include "buildParam.h"
#include "platform.h"
#include "renderer.h"
#include "utils.h"
#include "vulkanWindow.h"

#include <cstdlib>
#include <assert.h>
#include <vector>
#include <iostream>
#include <sstream>

Renderer::Renderer()
{
    setupDebugLayer();
    setupLayersAndExtensions();
    initInstance();
    enableDebug(); // After initInstance as we need instance :P
}

Renderer::~Renderer()
{
    delete vulkanWindow;
    destroyDevice();
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

const VkInstance Renderer::getVulkanInstance() const
{
    return instance;
}

const VkPhysicalDevice Renderer::getVulkanPhysicalDevice() const
{
    return gpuDetails.gpu;
}

const VkDevice Renderer::getVulkanDevice() const
{
    return device;
}

const VkQueue Renderer::getVulkanQueue() const
{
    return graphicsQueue;
}

const VkPhysicalDeviceProperties &Renderer::getVulkanPhysicalDeviceProperties() const
{
    return gpuDetails.properties;
}

const VkPhysicalDeviceMemoryProperties &Renderer::getVulkanPhysicalDeviceMemoryProperties() const
{
    return gpuDetails.memoryProperties;
}

const uint32_t Renderer::getGraphicsFamilyIndex() const
{
    return queueFamilyIndices.graphicsFamilyIndex;
}

VulkanWindow* Renderer::createVulkanVindow(uint32_t width, uint32_t height, std::string name, std::string title)
{
    vulkanWindow = new VulkanWindow(this, width, height, name, title);

    initDevice();
    initLogicalDevice();
    vulkanWindow->doPostInit();

    return vulkanWindow;
}

bool Renderer::run()
{
    if(vulkanWindow != nullptr)
    {
        return vulkanWindow->update();
    }

    return true;
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
    uint32_t familyCount = 0;
    uint32_t graphicsFamilyIndex = UINT32_MAX;
    uint32_t presentFamilyIndex = UINT32_MAX;

    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> familyPropertiesList(familyCount);
    std::vector<VkBool32> supportsPresentQueue(familyCount);

    for(uint32_t queueCounter = 0; queueCounter < familyCount; ++queueCounter)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, queueCounter, vulkanWindow->getSurface(), &supportsPresentQueue.data()[queueCounter]);
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

    queueFamilyIndices.graphicsFamilyIndex = graphicsFamilyIndex;
    queueFamilyIndices.presentFamilyIndex = presentFamilyIndex;
    queueFamilyIndices.hasSeparatePresentQueue = (presentFamilyIndex != graphicsFamilyIndex);

    std::cout<<"\n---------- Queue Family Indices ----------\n";
    std::cout<<"\nGraphics Family Index\t:"<<graphicsFamilyIndex<<"\n";
    std::cout<<"\nPresent Family Index\t:"<<presentFamilyIndex<<"\n";
    std::cout<<"\n---------- Queue Family Indices End ----------\n";

    return true;
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
