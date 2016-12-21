#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <string.h>

int main()
{
    VkInstanceCreateInfo instanceCreateInfo;
    VkInstance instance = 0;
    VkResult result;

    memset(&instanceCreateInfo, 0, sizeof(instanceCreateInfo));

    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = nullptr;
    instanceCreateInfo.flags = 0; // Must be 0 as per documentation.
    instanceCreateInfo.pApplicationInfo = NULL;
    instanceCreateInfo.enabledLayerCount = 0;
    instanceCreateInfo.ppEnabledLayerNames = NULL;
    instanceCreateInfo.enabledExtensionCount = 0;
    instanceCreateInfo.ppEnabledExtensionNames = NULL;

    result = vkCreateInstance(&instanceCreateInfo, NULL, &instance);

    if(result != VK_SUCCESS)
    {
        printf("Instance Create Error %d\n", result);
        return (EXIT_FAILURE);
    }

    printf("Instance create: %p\n", instance);
    vkDestroyInstance(instance, NULL);

    return (EXIT_SUCCESS);
}
