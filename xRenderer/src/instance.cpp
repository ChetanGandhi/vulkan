#include "instance.h"

namespace xr {
    Instance::Instance() {}
    Instance::~Instance()
    {
        vkDestroyInstance(this->vkInstance, VK_NULL_HANDLE);
        this->vkInstance = VK_NULL_HANDLE;
    }

    VkResult Instance::initVulkanInstance(VkApplicationInfo *applicationInfo, std::vector<const char*> *instanceLayers, std::vector<const char*> *instanceExtensions, VkDebugUtilsMessengerCreateInfoEXT *debugUtilsMessengerCreateInfo)
    {
        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pNext = debugUtilsMessengerCreateInfo;
        instanceCreateInfo.flags = 0;
        instanceCreateInfo.pApplicationInfo = applicationInfo;
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers->size());
        instanceCreateInfo.ppEnabledLayerNames = instanceLayers->data();
        instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions->size());
        instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions->data();

        return vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &this->vkInstance);
    }
}
