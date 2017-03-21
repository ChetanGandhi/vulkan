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
    initDevice();
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
        std::exit(-1);
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
    return gpu;
}

const VkDevice Renderer::getVulkanDevice() const
{
    return device;
}
const VkQueue Renderer::getVulkanQueue() const
{
    return queue;
}

const VkPhysicalDeviceProperties &Renderer::getVulkanPhysicalDeviceProperties() const
{
    return gpuProperties;
}

const VkPhysicalDeviceMemoryProperties &Renderer::getVulkanPhysicalDeviceMemoryProperties() const
{
    return gpuMemoryProperties;
}

const uint32_t Renderer::getGraphicsFamilyIndex() const
{
    return graphicsFamilyIndex;
}

VulkanWindow* Renderer::createVulkanVindow(uint32_t width, uint32_t height, std::string name, std::string title)
{
    vulkanWindow = new VulkanWindow(this, width, height, name, title);
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
    applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 3);
    applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    applicationInfo.pApplicationName = "Vulkan - Ceate Instance";
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
    checkError(result);
}

void Renderer::destroyInstance()
{
    vkDestroyInstance(instance, VK_NULL_HANDLE);
    instance = VK_NULL_HANDLE;
}

void Renderer::initDevice()
{
    bool found = false;

    {
        uint32_t gpuCount = 0;
        vkEnumeratePhysicalDevices(instance, &gpuCount, VK_NULL_HANDLE);
        std::vector<VkPhysicalDevice> gpuList(gpuCount);
        vkEnumeratePhysicalDevices(instance, &gpuCount, gpuList.data());

        for(uint32_t counter = 0; counter < gpuCount; ++counter)
        {
            VkPhysicalDevice nextGpu = gpuList[counter];
            VkPhysicalDeviceProperties nextGpuProperties {};
            VkPhysicalDeviceMemoryProperties nextGpuMemoryProperties {};

            vkGetPhysicalDeviceProperties(nextGpu, &nextGpuProperties);
            vkGetPhysicalDeviceMemoryProperties(nextGpu, &nextGpuMemoryProperties);

            // printGpuMemoryProperties();
            printGpuProperties(&nextGpuProperties, counter, gpuCount);

            uint32_t familyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(nextGpu, &familyCount, VK_NULL_HANDLE);
            std::vector<VkQueueFamilyProperties> familyPropertiesList(familyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(nextGpu, &familyCount, familyPropertiesList.data());

            for(uint32_t familyCounter = 0; familyCounter < familyCount; ++familyCounter)
            {
                if(familyPropertiesList[familyCounter].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    found = true;
                    graphicsFamilyIndex = familyCounter;
                    gpu = nextGpu;
                    gpuProperties = nextGpuProperties;
                    gpuMemoryProperties = nextGpuMemoryProperties;
                }
            }
        }
    }

    if(!found)
    {
        assert(0 && "Vulkan Error: Queue family supporting graphics not found.");
        std::exit(-1);
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
        vkEnumerateDeviceLayerProperties(gpu, &layerCount, VK_NULL_HANDLE);
        std::vector<VkLayerProperties> layerPropertiesList(layerCount);
        vkEnumerateDeviceLayerProperties(gpu, &layerCount, layerPropertiesList.data());
        printDeviceLayerProperties(layerPropertiesList);
    }

    float queuePriorities[] {1.0f};

    VkDeviceQueueCreateInfo deviceQueueCreateInfo {};
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.pNext = nullptr;
    deviceQueueCreateInfo.flags = 0;
    deviceQueueCreateInfo.queueFamilyIndex = graphicsFamilyIndex;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.pQueuePriorities = queuePriorities;

    VkDeviceCreateInfo deviceCreateInfo {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = nullptr;
    deviceCreateInfo.flags = 0;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.enabledLayerCount = deviceLayerList.size(); // Deprecated but still good for old API
    deviceCreateInfo.ppEnabledLayerNames = deviceLayerList.data(); // Deprecated but still good for old API
    deviceCreateInfo.enabledExtensionCount = deviceExtensionList.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionList.data();
    deviceCreateInfo.pEnabledFeatures = nullptr;

    VkResult result = vkCreateDevice(gpu, &deviceCreateInfo, VK_NULL_HANDLE, &device);
    checkError(result);

    vkGetDeviceQueue(device, graphicsFamilyIndex, 0, &queue);
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
