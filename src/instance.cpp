#include "instance.h"

Instance::Instance(std::vector<const char*> instanceLayers, std::vector<const char*> instanceExtensions)
{
    this->instanceLayers = instanceLayers;
    this->instanceExtensions = instanceExtensions;

    this->debugger = new Debugger();
    if(this->debugger->checkValidationLayerSupport())
    {
        this->instanceExtensions.push_back(this->debugger->validationLayerName);
    }
    else
    {
        logf("Validation layer not supported!!!");
    }
}

Instance::~Instance()
{
    this->debugger->destory(&(this->vkInstance));
    delete this->debugger;
    this->debugger = nullptr;

    vkDestroyInstance(this->vkInstance, VK_NULL_HANDLE);
    this->vkInstance = VK_NULL_HANDLE;
}

VkResult Instance::initVulkanInstance(VkApplicationInfo *applicationInfo)
{
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    #ifndef NDEBUG

    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo;
    this->debugger->createInfo(debugUtilsMessengerCreateInfo);
    instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfo;

    #endif

    instanceCreateInfo.flags = 0;
    instanceCreateInfo.pApplicationInfo = applicationInfo;
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(this->instanceLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = this->instanceLayers.data();
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(this->instanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = this->instanceExtensions.data();

    return vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &this->vkInstance);
}
