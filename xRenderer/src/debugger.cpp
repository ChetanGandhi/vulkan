#include "debugger.h"

namespace xr {
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugMessangerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
    {
        std::ostringstream stream;

        if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            stream<<"[ERROR] | ";
        }
        else if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            stream<<"[WARNING] | ";
        }
        else if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
            stream<<"[INFO] | ";
        }
        else if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        {
            stream<<"[VERBOSE] | ";
        }

        if(messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
        {
            stream<<"[GENERAL] | ";
        }
        else if(messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        {
            stream<<"[VALIDATION] | ";
        }
        else if(messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        {
            stream<<"[PERFORMANCE] | ";
        }

        stream<<pCallbackData->pMessage;
        logf(stream.str().c_str());

        #if defined (_WIN32)

            if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            {
                MessageBox(NULL, stream.str().c_str(), TEXT("Vulkan Error"), MB_OK | MB_ICONERROR);
            }

        #endif // _WIN32

        return false;
    }

    bool Debugger::checkValidationLayerSupport()
    {
        bool layerFound = false;

        #ifndef NDEBUG

        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for(const VkLayerProperties& layerProperties : availableLayers)
        {
            if(strcmp(this->validationLayerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        #endif

        return layerFound;
    }

    void Debugger::fillCreateInfo(VkDebugUtilsMessengerCreateInfoEXT *createInfo)
    {
        createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo->pNext = nullptr;
        createInfo->flags = 0;
        createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        createInfo->pfnUserCallback = xr::debugMessangerCallback;
        createInfo->pUserData = nullptr;
        createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        #ifdef ENABLE_DEBUG_REPORT_VERBOSE_BIT

        createInfo->messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

        #endif

        #ifdef ENABLE_DEBUG_REPORT_INFORMATION_BIT

        createInfo->messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

        #endif
    }

    VkResult Debugger::initialize(VkInstance *instance, VkDebugUtilsMessengerCreateInfoEXT *createInfo)
    {
        #ifndef NDEBUG

        PFN_vkCreateDebugUtilsMessengerEXT _vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT");

        if (_vkCreateDebugUtilsMessengerEXT == nullptr)
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
        else
        {
            return _vkCreateDebugUtilsMessengerEXT(*instance, createInfo, nullptr, &(this->pDebugMessenger));
        }

        #endif

        return VK_SUCCESS;
    }

    void Debugger::destory(VkInstance *instance)
    {
        #ifndef NDEBUG

        PFN_vkDestroyDebugUtilsMessengerEXT _vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(*instance, "vkDestroyDebugUtilsMessengerEXT");

        if (_vkDestroyDebugUtilsMessengerEXT != nullptr)
        {
            _vkDestroyDebugUtilsMessengerEXT(*instance, this->pDebugMessenger, nullptr);
            _vkDestroyDebugUtilsMessengerEXT = nullptr;
        }

        #endif
    }
}
