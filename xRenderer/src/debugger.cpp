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
    XR_LOG(context->logger, severity, "(%d) | [Object: %d] | %s", messageCode, object, message);

    return false;
}

XR_API VkResult xrCreateDebugger(XrContext *context, VkDebugReportFlagsEXT flags)
{
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
    createInfo.flags = flags;
    createInfo.pfnCallback = xrDebugMessengerCallback;
    createInfo.pUserData = static_cast<void *>(context);

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
