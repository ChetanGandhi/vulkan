#include "instance.h"

VkResult xrCreateVulkanInstance(
    XrContext *context,
    VkApplicationInfo *applicationInfo,
    char **instanceLayers,
    uint32_t instanceLayersCount,
    char **instanceExtensions,
    uint32_t instanceExtensionsCount
)
{
    VkInstanceCreateInfo instanceCreateInfo;
    memset((void *)&instanceCreateInfo, 0, sizeof(VkInstanceCreateInfo));
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = VK_NULL_HANDLE;
    instanceCreateInfo.flags = 0;
    instanceCreateInfo.pApplicationInfo = applicationInfo;
    instanceCreateInfo.enabledLayerCount = instanceLayersCount;
    instanceCreateInfo.ppEnabledLayerNames = instanceLayers;
    instanceCreateInfo.enabledExtensionCount = instanceExtensionsCount;
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions;

    return vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &(context->instance));
}

void xrDestroyVulkanInstance(XrContext *context)
{
    if (context->instance)
    {
        vkDestroyInstance(context->instance, VK_NULL_HANDLE);
        context->instance = VK_NULL_HANDLE;
    }
}
