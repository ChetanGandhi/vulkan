#pragma once

#include "platform.h"

namespace xr {
    class Debugger {
        public:
            const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

            VkResult initialize(VkInstance *instance, VkDebugUtilsMessengerCreateInfoEXT *createInfo);
            void destory(VkInstance *instance);

            bool checkValidationLayerSupport();
            void createInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

        private:
            VkDebugUtilsMessengerEXT pDebugMessenger;
    };
}
