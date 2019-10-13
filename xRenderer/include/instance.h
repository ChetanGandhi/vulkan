#pragma once

#include "platform.h"
#include "debugger.h"

namespace xr {
    class Instance
    {
    public:
        Instance(std::vector<const char*> instanceLayers, std::vector<const char*> instanceExtensions);
        ~Instance();

        VkResult initVulkanInstance(VkApplicationInfo *applicationInfo);
        VkInstance vkInstance = VK_NULL_HANDLE;

    private:
        std::vector<const char*> instanceLayers;
        std::vector<const char*> instanceExtensions;

        Debugger *debugger = nullptr;

        void setupDebugging();
    };
}
