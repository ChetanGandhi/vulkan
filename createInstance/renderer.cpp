#include "renderer.h"
#include "buildParam.h"
#include "utils.h"
#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <sstream>

#ifdef _WIN32

#include<windows.h>

#endif

PFN_vkCreateDebugReportCallbackEXT _vkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT _vkDestroyDebugReportCallbackEXT = nullptr;

Renderer::Renderer()
{
    setupDebugLayer();
    initInstance();
    enableDebud(); // After initInstance as we need instance :P
    initDevice();
}

Renderer::~Renderer()
{
    destroyDevice();
    disableDebug();
    destroyInstance();
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t sourceObject, size_t location, int32_t messageCode, const char *layerPrefix, const char *message, void *userData)
{
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

    #endif

    return false;
}

void Renderer::setupDebugLayer()
{
    debugReportCallbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    debugReportCallbackInfo.pfnCallback = debugReportCallback;
    debugReportCallbackInfo.pNext = NULL;
    debugReportCallbackInfo.pUserData = NULL;
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
}

void Renderer::enableDebud()
{
    _vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    _vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");

    if(_vkCreateDebugReportCallbackEXT == nullptr || _vkDestroyDebugReportCallbackEXT == nullptr)
    {
        assert(0 && "Vulkan Error: Cannot fetch debug functions");
        std::exit(-1);
    }

    _vkCreateDebugReportCallbackEXT(instance, &debugReportCallbackInfo, nullptr, &debugReport);
}

void Renderer::disableDebug()
{
    _vkDestroyDebugReportCallbackEXT(instance, debugReport, nullptr);
    debugReport = nullptr;
}

void Renderer::initInstance()
{
    VkApplicationInfo applicationInfo {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext = NULL;
    applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 3);
    applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    applicationInfo.pApplicationName = "Vulkan - Ceate Instance";
    applicationInfo.pEngineName = NULL;
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

    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
    checkError(result);
}

void Renderer::destroyInstance()
{
    vkDestroyInstance(instance, nullptr);
    instance = nullptr;
}

void Renderer::initDevice()
{
    {
        uint32_t gpuCount = 0;
        vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
        std::vector<VkPhysicalDevice> gpuList(gpuCount);
        vkEnumeratePhysicalDevices(instance, &gpuCount, gpuList.data());
        gpu = gpuList[0];

        vkGetPhysicalDeviceProperties(gpu, &gpuProperties);
        printGpuProperties(&gpuProperties);
    }

    {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> familyPropertiesList(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, familyPropertiesList.data());

        bool found = false;

        for(uint32_t counter = 0; counter < familyCount; ++counter)
        {
            if(familyPropertiesList[counter].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                found = true;
                graphicsFamilyIndex = counter;
            }
        }


        if(!found)
        {
            assert(0 && "Vulkan Error: Queue family supporting graphics not found.");
            std::exit(-1);
        }
    }

    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> layerPropertiesList(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layerPropertiesList.data());
        printInstanceLayerProperties(layerPropertiesList);
    }

    {
        uint32_t layerCount = 0;
        vkEnumerateDeviceLayerProperties(gpu, &layerCount, nullptr);
        std::vector<VkLayerProperties> layerPropertiesList(layerCount);
        vkEnumerateDeviceLayerProperties(gpu, &layerCount, layerPropertiesList.data());
        printDeviceLayerProperties(layerPropertiesList);
    }

    float queuePriorities[] {1.0f};

    VkDeviceQueueCreateInfo deviceQueueCreateInfo {};
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.pNext = NULL;
    deviceQueueCreateInfo.flags = 0;
    deviceQueueCreateInfo.queueFamilyIndex = graphicsFamilyIndex;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.pQueuePriorities = queuePriorities;

    VkDeviceCreateInfo deviceCreateInfo {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = NULL;
    deviceCreateInfo.flags = 0;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.enabledLayerCount = deviceLayerList.size();
    deviceCreateInfo.ppEnabledLayerNames = deviceLayerList.data();
    deviceCreateInfo.enabledExtensionCount = deviceExtensionList.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionList.data();
    deviceCreateInfo.pEnabledFeatures = NULL;

    VkResult result = vkCreateDevice(gpu, &deviceCreateInfo, nullptr, &device);
    checkError(result);
}

void Renderer::destroyDevice()
{
    vkDestroyDevice(device, nullptr);
    device = nullptr;
}

void Renderer::printGpuProperties(VkPhysicalDeviceProperties *properties)
{
    if(!properties)
    {
        std::cout<<"No GPU properties to show!!!"<<std::endl;
        return;
    }

    std::cout<<"\n---------- GPU Properties ----------\n";
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
