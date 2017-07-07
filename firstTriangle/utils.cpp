#pragma once

#include "buildParam.h"
#include "utils.h"

#include <fstream>

void checkError(VkResult result, std::string file, uint32_t lineNumber)
{
    #if ENABLE_RUNTIME_DEBUG

    if(result < 0)
    {
        switch(result)
        {
            case VK_ERROR_OUT_OF_HOST_MEMORY: // -1
                std::cout<<"Vulkan Runtime Error [VK_ERROR_OUT_OF_HOST_MEMORY|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            case VK_ERROR_OUT_OF_DEVICE_MEMORY: // -2
                std::cout<<"Vulkan Runtime Error [VK_ERROR_OUT_OF_DEVICE_MEMORY|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            case VK_ERROR_INITIALIZATION_FAILED: // -3
                std::cout<<"Vulkan Runtime Error [VK_ERROR_INITIALIZATION_FAILED|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            case VK_ERROR_DEVICE_LOST: // -4
                std::cout<<"Vulkan Runtime Error [VK_ERROR_DEVICE_LOST|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            case VK_ERROR_MEMORY_MAP_FAILED: // -5
                std::cout<<"Vulkan Runtime Error [VK_ERROR_MEMORY_MAP_FAILED|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            case VK_ERROR_LAYER_NOT_PRESENT: // -6
                std::cout<<"Vulkan Runtime Error [VK_ERROR_LAYER_NOT_PRESENT|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            case VK_ERROR_EXTENSION_NOT_PRESENT: // -7
                std::cout<<"Vulkan Runtime Error [VK_ERROR_EXTENSION_NOT_PRESENT|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            case VK_ERROR_FEATURE_NOT_PRESENT: // -8
                std::cout<<"Vulkan Runtime Error [VK_ERROR_FEATURE_NOT_PRESENT|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            case VK_ERROR_INCOMPATIBLE_DRIVER: // -9
                std::cout<<"Vulkan Runtime Error [VK_ERROR_INCOMPATIBLE_DRIVER|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            case VK_ERROR_TOO_MANY_OBJECTS: // -10
                std::cout<<"Vulkan Runtime Error [VK_ERROR_TOO_MANY_OBJECTS|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            case VK_ERROR_FORMAT_NOT_SUPPORTED: // -11
                std::cout<<"Vulkan Runtime Error [VK_ERROR_FORMAT_NOT_SUPPORTED|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            case VK_ERROR_SURFACE_LOST_KHR: // -1000000000
                std::cout<<"Vulkan Runtime Error [VK_ERROR_SURFACE_LOST_KHR|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: // -1000000001
                std::cout<<"Vulkan Runtime Error [VK_ERROR_NATIVE_WINDOW_IN_USE_KHR|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            case VK_ERROR_OUT_OF_DATE_KHR: // -1000001004
                std::cout<<"Vulkan Runtime Error [VK_ERROR_OUT_OF_DATE_KHR|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: // -1000003001
                std::cout<<"Vulkan Runtime Error [VK_ERROR_INCOMPATIBLE_DISPLAY_KHR|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            case VK_ERROR_VALIDATION_FAILED_EXT: // -1000011001
                std::cout<<"Vulkan Runtime Error [VK_ERROR_VALIDATION_FAILED_EXT|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            case VK_ERROR_INVALID_SHADER_NV: // -1000012000
                std::cout<<"Vulkan Runtime Error [VK_ERROR_INVALID_SHADER_NV|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;

            default:
                std::cout<<"Vulkan Runtime Error [Unknown Error|"<<result<<"]\n"<<"File :"<<file<<"\nLine: "<<lineNumber<<"\n";
            break;
        }

        assert(0 && "----- Vulkan Runtime Error -----");
    }

    #endif // ENABLE_DEBUG
}

uint32_t findMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties *gpuMemoryProperties, const VkMemoryRequirements *imageMemoryRequirements, const VkMemoryPropertyFlags requiredMemoryProperties)
{
    for(uint32_t memoryTypeCounter = 0; memoryTypeCounter < gpuMemoryProperties->memoryTypeCount; ++memoryTypeCounter)
    {
        if(imageMemoryRequirements->memoryTypeBits & (1 << memoryTypeCounter))
        {
            if((gpuMemoryProperties->memoryTypes[memoryTypeCounter].propertyFlags & requiredMemoryProperties) == requiredMemoryProperties)
            {
                return memoryTypeCounter;
            }

        }
    }

    assert(0 && "Could not find proper memory type.");
    return UINT32_MAX;
}

bool readFile(const std::string &fileName, std::vector<char> *data)
{
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);

    if(!file.is_open())
    {
        std::cout<<"Onable to open file: "<<fileName<<"\n";
        return false;
    }

    size_t fileSize = (size_t) file.tellg();
    data->resize(fileSize);

    file.seekg(0);
    file.read(data->data(), fileSize);
    file.close();

    return true;
}
