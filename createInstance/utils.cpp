#include "buildParam.h"
#include "utils.h"

void checkError(VkResult result)
{
    #if ENABLE_RUNTIME_DEBUG

    if(result < 0)
    {
        switch(result)
        {
            VK_ERROR_OUT_OF_HOST_MEMORY:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_OUT_OF_HOST_MEMORY|"<<result<<"]\n";
            break;

            VK_ERROR_OUT_OF_DEVICE_MEMORY:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_OUT_OF_DEVICE_MEMORY|"<<result<<"]\n";
            break;

            VK_ERROR_INITIALIZATION_FAILED:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_INITIALIZATION_FAILED|"<<result<<"]\n";
            break;

            VK_ERROR_DEVICE_LOST:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_DEVICE_LOST|"<<result<<"]\n";
            break;

            VK_ERROR_MEMORY_MAP_FAILED:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_MEMORY_MAP_FAILED|"<<result<<"]\n";
            break;

            VK_ERROR_LAYER_NOT_PRESENT:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_LAYER_NOT_PRESENT|"<<result<<"]\n";
            break;

            VK_ERROR_EXTENSION_NOT_PRESENT:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_EXTENSION_NOT_PRESENT|"<<result<<"]\n";
            break;

            VK_ERROR_FEATURE_NOT_PRESENT:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_FEATURE_NOT_PRESENT|"<<result<<"]\n";
            break;

            VK_ERROR_INCOMPATIBLE_DRIVER:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_INCOMPATIBLE_DRIVER|"<<result<<"]\n";
            break;

            VK_ERROR_TOO_MANY_OBJECTS:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_TOO_MANY_OBJECTS|"<<result<<"]\n";
            break;

            VK_ERROR_FORMAT_NOT_SUPPORTED:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_FORMAT_NOT_SUPPORTED|"<<result<<"]\n";
            break;

            VK_ERROR_SURFACE_LOST_KHR:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_SURFACE_LOST_KHR|"<<result<<"]\n";
            break;

            VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_NATIVE_WINDOW_IN_USE_KHR|"<<result<<"]\n";
            break;

            VK_ERROR_OUT_OF_DATE_KHR:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_OUT_OF_DATE_KHR|"<<result<<"]\n";
            break;

            VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_INCOMPATIBLE_DISPLAY_KHR|"<<result<<"]\n";
            break;

            VK_ERROR_VALIDATION_FAILED_EXT:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_VALIDATION_FAILED_EXT|"<<result<<"]\n";
            break;

            VK_ERROR_INVALID_SHADER_NV:
            std::cout<<"Vulkan Runtime Error [VK_ERROR_INVALID_SHADER_NV|"<<result<<"]\n";
            break;

            default:
            std::cout<<"Vulkan Runtime Error [Unknown Error|"<<result<<"]\n";
            break;
        }

        assert(0 && "Vulkan Runtime Error");
    }

    #endif // ENABLE_DEBUG
}
