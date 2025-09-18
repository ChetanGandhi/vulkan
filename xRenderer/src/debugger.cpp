#include "debugger.h"
#include "logger.h"
#include "utils.h"

VKAPI_ATTR VkBool32 VKAPI_CALL xrDebugMessengerCallback(
    VkDebugReportFlagsEXT debugReportFlagsEXT,
    VkDebugReportObjectTypeEXT debugReportObjectTypeEXT,
    uint64_t object,
    size_t location,
    int32_t messageCode,
    const char *layerPrefix,
    const char *message,
    void *userData
)
{
    const char *severity = "";

    if ((debugReportFlagsEXT & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) == VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
    {
        severity = "INFORMATION";
    }
    else if ((debugReportFlagsEXT & VK_DEBUG_REPORT_WARNING_BIT_EXT) == VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        severity = "WARNING";
    }
    else if ((debugReportFlagsEXT & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) == VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
    {
        severity = "PERFORMANCE_WARNING";
    }
    else if ((debugReportFlagsEXT & VK_DEBUG_REPORT_ERROR_BIT_EXT) == VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        severity = "ERROR";
    }
    else if ((debugReportFlagsEXT & VK_DEBUG_REPORT_DEBUG_BIT_EXT) == VK_DEBUG_REPORT_DEBUG_BIT_EXT)
    {
        severity = "DEBUG";
    }

    XrContext *context = static_cast<XrContext *>(userData);
    XR_LOG(context->logger, severity, layerPrefix, " (", messageCode, ") ", message);

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
    memset((void *)vkLayerProperties, 0, sizeof(VkLayerProperties) * validationLayerCount);
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

XR_API VkResult xrCreateDebugger(XrContext *context)
{
#ifndef XR_ENABLE_RUNTIME_DEBUG
    return VK_SUCCESS;
#endif

    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
        (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(context->instance, "vkCreateDebugReportCallbackEXT");

    if (vkCreateDebugReportCallbackEXT == NULL)
    {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkDebugReportCallbackCreateInfoEXT createInfo;
    memset((void *)&createInfo, 0, sizeof(VkDebugReportCallbackCreateInfoEXT));
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.pNext = VK_NULL_HANDLE;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    createInfo.pfnCallback = VK_NULL_HANDLE;
    createInfo.pUserData = NULL;

#ifdef XR_ENABLE_DEBUG_REPORT_LOGGING

    createInfo.pfnCallback = xrDebugMessengerCallback;
    createInfo.pUserData = static_cast<void *>(context);
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;

#endif

#ifdef ENABLE_DEBUG_REPORT_VERBOSE_BIT

    createInfo->messageSeverity |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;

#endif

#ifdef ENABLE_DEBUG_REPORT_INFORMATION_BIT

    createInfo->messageSeverity |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;

#endif
    return vkCreateDebugReportCallbackEXT(context->instance, &createInfo, VK_NULL_HANDLE, &context->debugReportCallback);
}

XR_API VkResult xrDestroyDebugger(XrContext *context)
{
    if (context->debugReportCallback)
    {
        PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
            (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(context->instance, "vkDestroyDebugReportCallbackEXT");

        if (!vkDestroyDebugReportCallbackEXT)
        {
            return VK_ERROR_UNKNOWN;
        }

        vkDestroyDebugReportCallbackEXT(context->instance, context->debugReportCallback, VK_NULL_HANDLE);
        vkDestroyDebugReportCallbackEXT = VK_NULL_HANDLE;
        context->debugReportCallback = VK_NULL_HANDLE;

        XR_LOG_INFO(context->logger, "Debugger destroyed");
    }

    return VK_SUCCESS;
}
