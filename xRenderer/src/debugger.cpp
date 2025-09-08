#include "debugger.h"
#include "logger.h"
#include "utils.h"
#include "context.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL xrDebugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData
)
{
    const char *severity = "";
    const char *type = "";

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        severity = "ERROR";
    }
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        severity = "WARNING";
    }
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        severity = "INFO";
    }
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        severity = "VERBOSE";
    }

    if (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
    {
        type = "[GENERAL]";
    }
    else if (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
    {
        type = "[VALIDATION]";
    }
    else if (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
    {
        type = "[PERFORMANCE]";
    }

    XrContext *context = static_cast<XrContext *>(pUserData);
    XR_LOG(context->logger, severity, type, " | ", pCallbackData->pMessage);

    return false;
}

XR_API VkResult xrIsValidationLayerSupport(XrContext *context, const char *validationLayerName)
{
#ifndef XR_ENABLE_RUNTIME_DEBUG
    return VK_ERROR_LAYER_NOT_PRESENT;
#endif

    VkResult vkResult = VK_SUCCESS;
    uint32_t validationLayerCount = 0;
    vkResult = vkEnumerateInstanceLayerProperties(&validationLayerCount, VK_NULL_HANDLE);

    if (XR_IS_ERROR(vkResult))
    {
        return vkResult;
    }

    XR_LOG_INFO(context->logger, "Validation layer count: %d", validationLayerCount);

    VkLayerProperties *vkLayerProperties = (VkLayerProperties *)malloc(sizeof(VkLayerProperties) * validationLayerCount);
    memset((void *)&vkLayerProperties, 0, sizeof(VkLayerProperties) * validationLayerCount);
    vkResult = vkEnumerateInstanceLayerProperties(&validationLayerCount, vkLayerProperties);

    if (XR_IS_ERROR(vkResult))
    {
        if (vkLayerProperties)
        {
            free(vkLayerProperties);
            vkLayerProperties = VK_NULL_HANDLE;

            XR_LOG_INFO(context->logger, "Free layer properties array");
        }

        return vkResult;
    }

    for (uint32_t index = 0; index < validationLayerCount; ++index)
    {
        XR_LOG_INFO(context->logger, "Validation layer name [%d]: %s", index, vkLayerProperties[index].layerName);
    }

    VkBool32 validationLayerFound = VK_FALSE;

    for (uint32_t index = 0; index < validationLayerCount; ++index)
    {
        if (strcmp(vkLayerProperties[index].layerName, validationLayerName) == 0)
        {
            validationLayerFound = VK_TRUE;
            break;
        }
    }

    if (vkLayerProperties)
    {
        free(vkLayerProperties);
        vkLayerProperties = VK_NULL_HANDLE;

        XR_LOG_INFO(context->logger, "Free layer properties array");
    }

    if (!validationLayerFound)
    {
        return VK_ERROR_LAYER_NOT_PRESENT;
    }

    return vkResult;
}

XR_API void xrFillDebuggerCreateInfo(XrContext *context, VkDebugUtilsMessengerCreateInfoEXT *createInfo)
{
    createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo->pNext = nullptr;
    createInfo->flags = 0;

#ifdef XR_ENABLE_DEBUG_REPORT_LOGGING
    createInfo->pfnUserCallback = xrDebugMessengerCallback;
    createInfo->pUserData = static_cast<void *>(context);

    createInfo->messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

#endif

#ifdef ENABLE_DEBUG_REPORT_VERBOSE_BIT

    createInfo->messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

#endif

#ifdef ENABLE_DEBUG_REPORT_INFORMATION_BIT

    createInfo->messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

#endif
}

XR_API VkResult xrCreateDebugger(XrContext *context, VkDebugUtilsMessengerCreateInfoEXT *createInfo)
{
#ifndef XR_ENABLE_RUNTIME_DEBUG
    return VK_ERROR_EXTENSION_NOT_PRESENT;
#endif

    PFN_vkCreateDebugUtilsMessengerEXT _vkCreateDebugUtilsMessengerEXT =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance, "vkCreateDebugUtilsMessengerEXT");

    if (!_vkCreateDebugUtilsMessengerEXT)
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    return _vkCreateDebugUtilsMessengerEXT(context->instance, createInfo, VK_NULL_HANDLE, &(context->pDebugMessenger));
}

XR_API VkResult xrDestroyDebugger(XrContext *context)
{
    if (context->pDebugMessenger)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT _vkDestroyDebugUtilsMessengerEXT =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance, "vkDestroyDebugUtilsMessengerEXT");

        if (_vkDestroyDebugUtilsMessengerEXT)
        {
            _vkDestroyDebugUtilsMessengerEXT(context->instance, context->pDebugMessenger, VK_NULL_HANDLE);
            _vkDestroyDebugUtilsMessengerEXT = VK_NULL_HANDLE;
            context->pDebugMessenger = VK_NULL_HANDLE;

            XR_LOG_INFO(context->logger, "Debugger destroyed");
        }
        else
        {
            return VK_ERROR_UNKNOWN;
        }
    }

    return VK_SUCCESS;
}
