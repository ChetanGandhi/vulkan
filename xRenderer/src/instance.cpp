#include "instance.h"

VkResult xrCreateVulkanInstance(
    XrContext *context,
    VkApplicationInfo *applicationInfo,
    std::vector<const char *> *instanceLayers,
    std::vector<const char *> *instanceExtensions
)
{
    VkInstanceCreateInfo instanceCreateInfo;
    memset((void *)&instanceCreateInfo, 0, sizeof(VkInstanceCreateInfo));
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = VK_NULL_HANDLE;
    instanceCreateInfo.flags = 0;
    instanceCreateInfo.pApplicationInfo = applicationInfo;
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers->size());
    instanceCreateInfo.ppEnabledLayerNames = instanceLayers->data();
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions->size());
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions->data();

    return vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &(context->instance));
}

VkResult xrDestroyVulkanInstance(XrContext *context)
{
    if (context->instance)
    {
        vkDestroyInstance(context->instance, VK_NULL_HANDLE);
        context->instance = VK_NULL_HANDLE;
    }

    return VK_SUCCESS;
}
