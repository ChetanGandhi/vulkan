#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "vulkan-1.lib")

int main()
{
    VkInstanceCreateInfo instanceCreateInfo;
    VkInstance instance = 0;
    VkResult result;

    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = NULL;
    instanceCreateInfo.pApplicationInfo = NULL;
    instanceCreateInfo.enabledLayerCount = 0;
    instanceCreateInfo.ppEnabledLayerNames = NULL;
    instanceCreateInfo.enabledExtensionCount = 0;
    instanceCreateInfo.ppEnabledExtensionNames = NULL;

    result = vkCreateInstance(&instanceCreateInfo, NULL, &instance);

    if(result != VK_SUCCESS)
    {
        printf("Error %d\n", result);
        return (EXIT_FAILURE);
    }

    printf("Instance create: %p\n", instance);
    vkDestroyInstance(instance, NULL);

    return (EXIT_SUCCESS);
}
