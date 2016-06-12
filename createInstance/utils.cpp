#include "buildParam.h"
#include "utils.h"

void checkError(VkResult result)
{
    #if ENABLE_RUNTIME_DEBUG

    if(result < 0)
    {
        switch(result)
        {
            case VK_ERROR_OUT_OF_HOST_MEMORY: // -1
            std::cout<<"Vulkan Runtime Error [VK_ERROR_OUT_OF_HOST_MEMORY|"<<result<<"]\n";
            break;

            case VK_ERROR_OUT_OF_DEVICE_MEMORY: // -2
            std::cout<<"Vulkan Runtime Error [VK_ERROR_OUT_OF_DEVICE_MEMORY|"<<result<<"]\n";
            break;

            case VK_ERROR_INITIALIZATION_FAILED: // -3
            std::cout<<"Vulkan Runtime Error [VK_ERROR_INITIALIZATION_FAILED|"<<result<<"]\n";
            break;

            case VK_ERROR_DEVICE_LOST: // -4
            std::cout<<"Vulkan Runtime Error [VK_ERROR_DEVICE_LOST|"<<result<<"]\n";
            break;

            case VK_ERROR_MEMORY_MAP_FAILED: // -5
            std::cout<<"Vulkan Runtime Error [VK_ERROR_MEMORY_MAP_FAILED|"<<result<<"]\n";
            break;

            case VK_ERROR_LAYER_NOT_PRESENT: // -6
            std::cout<<"Vulkan Runtime Error [VK_ERROR_LAYER_NOT_PRESENT|"<<result<<"]\n";
            break;

            case VK_ERROR_EXTENSION_NOT_PRESENT: // -7
            std::cout<<"Vulkan Runtime Error [VK_ERROR_EXTENSION_NOT_PRESENT|"<<result<<"]\n";
            break;

            case VK_ERROR_FEATURE_NOT_PRESENT: // -8
            std::cout<<"Vulkan Runtime Error [VK_ERROR_FEATURE_NOT_PRESENT|"<<result<<"]\n";
            break;

            case VK_ERROR_INCOMPATIBLE_DRIVER: // -9
            std::cout<<"Vulkan Runtime Error [VK_ERROR_INCOMPATIBLE_DRIVER|"<<result<<"]\n";
            break;

            case VK_ERROR_TOO_MANY_OBJECTS: // -10
            std::cout<<"Vulkan Runtime Error [VK_ERROR_TOO_MANY_OBJECTS|"<<result<<"]\n";
            break;

            case VK_ERROR_FORMAT_NOT_SUPPORTED: // -11
            std::cout<<"Vulkan Runtime Error [VK_ERROR_FORMAT_NOT_SUPPORTED|"<<result<<"]\n";
            break;

            case VK_ERROR_SURFACE_LOST_KHR: // -1000000000
            std::cout<<"Vulkan Runtime Error [VK_ERROR_SURFACE_LOST_KHR|"<<result<<"]\n";
            break;

            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: // -1000000001
            std::cout<<"Vulkan Runtime Error [VK_ERROR_NATIVE_WINDOW_IN_USE_KHR|"<<result<<"]\n";
            break;

            case VK_ERROR_OUT_OF_DATE_KHR: // -1000001004
            std::cout<<"Vulkan Runtime Error [VK_ERROR_OUT_OF_DATE_KHR|"<<result<<"]\n";
            break;

            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: // -1000003001
            std::cout<<"Vulkan Runtime Error [VK_ERROR_INCOMPATIBLE_DISPLAY_KHR|"<<result<<"]\n";
            break;

            case VK_ERROR_VALIDATION_FAILED_EXT: // -1000011001
            std::cout<<"Vulkan Runtime Error [VK_ERROR_VALIDATION_FAILED_EXT|"<<result<<"]\n";
            break;

            case VK_ERROR_INVALID_SHADER_NV: // -1000012000
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
