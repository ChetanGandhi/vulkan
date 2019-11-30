#pragma once

#include "platform.h"
#include "debugger.h"

namespace xr {
    class Instance
    {
    public:
        Instance();
        ~Instance();

        VkResult initVulkanInstance(VkApplicationInfo *applicationInfo, std::vector<const char*> *instanceLayers, std::vector<const char*> *instanceExtensions, VkDebugUtilsMessengerCreateInfoEXT *debugUtilsMessengerCreateInfo);
        VkInstance vkInstance = VK_NULL_HANDLE;
    };
}
