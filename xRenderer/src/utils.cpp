
#include "platform.h"
#include "utils.h"
#include "logger.h"

void checkError(VkResult result, std::string file, uint32_t lineNumber)
{
    std::ostringstream stream;

    if(result < 0)
    {
        stream<<"Vulkan Runtime Error ";

        switch(result)
        {
            case VK_ERROR_OUT_OF_HOST_MEMORY: // -1
                stream<<"[VK_ERROR_OUT_OF_HOST_MEMORY|"<<result<<"]";
            break;

            case VK_ERROR_OUT_OF_DEVICE_MEMORY: // -2
                stream<<" [VK_ERROR_OUT_OF_DEVICE_MEMORY|"<<result<<"]";
            break;

            case VK_ERROR_INITIALIZATION_FAILED: // -3
                stream<<" [VK_ERROR_INITIALIZATION_FAILED|"<<result<<"]";
            break;

            case VK_ERROR_DEVICE_LOST: // -4
                stream<<" [VK_ERROR_DEVICE_LOST|"<<result<<"]";
            break;

            case VK_ERROR_MEMORY_MAP_FAILED: // -5
                stream<<" [VK_ERROR_MEMORY_MAP_FAILED|"<<result<<"]";
            break;

            case VK_ERROR_LAYER_NOT_PRESENT: // -6
                stream<<" [VK_ERROR_LAYER_NOT_PRESENT|"<<result<<"]";
            break;

            case VK_ERROR_EXTENSION_NOT_PRESENT: // -7
                stream<<" [VK_ERROR_EXTENSION_NOT_PRESENT|"<<result<<"]";
            break;

            case VK_ERROR_FEATURE_NOT_PRESENT: // -8
                stream<<" [VK_ERROR_FEATURE_NOT_PRESENT|"<<result<<"]";
            break;

            case VK_ERROR_INCOMPATIBLE_DRIVER: // -9
                stream<<" [VK_ERROR_INCOMPATIBLE_DRIVER|"<<result<<"]";
            break;

            case VK_ERROR_TOO_MANY_OBJECTS: // -10
                stream<<" [VK_ERROR_TOO_MANY_OBJECTS|"<<result<<"]";
            break;

            case VK_ERROR_FORMAT_NOT_SUPPORTED: // -11
                stream<<" [VK_ERROR_FORMAT_NOT_SUPPORTED|"<<result<<"]";
            break;

            case VK_ERROR_SURFACE_LOST_KHR: // -1000000000
                stream<<" [VK_ERROR_SURFACE_LOST_KHR|"<<result<<"]";
            break;

            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: // -1000000001
                stream<<" [VK_ERROR_NATIVE_WINDOW_IN_USE_KHR|"<<result<<"]";
            break;

            case VK_ERROR_OUT_OF_DATE_KHR: // -1000001004
                stream<<" [VK_ERROR_OUT_OF_DATE_KHR|"<<result<<"]";
            break;

            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: // -1000003001
                stream<<" [VK_ERROR_INCOMPATIBLE_DISPLAY_KHR|"<<result<<"]";
            break;

            case VK_ERROR_VALIDATION_FAILED_EXT: // -1000011001
                stream<<" [VK_ERROR_VALIDATION_FAILED_EXT|"<<result<<"]";
            break;

            case VK_ERROR_INVALID_SHADER_NV: // -1000012000
                stream<<" [VK_ERROR_INVALID_SHADER_NV|"<<result<<"]";
            break;

            default:
                stream<<" [Unknown Error|"<<result<<"]";
            break;
        }

        stream<<"\nFile :"<<file<<"\nLine: "<<lineNumber;
        logf(stream.str());

        assert(0 && "----- Vulkan Runtime Error -----");
    }
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
        std::cout<<"Unable to open file: "<<fileName<<"\n";
        return false;
    }

    size_t fileSize = (size_t) file.tellg();
    data->resize(fileSize);

    file.seekg(0);
    file.read(data->data(), fileSize);
    file.close();

    return true;
}

size_t currentDateTime(char *dateTimeString, size_t size)
{
    time_t now = time(NULL);
    struct tm tmStruct;

    #if defined (_WIN32) // check for Windows

    _localtime64_s(&tmStruct, &now);

    #elif defined (__linux) // check for Linux

    localtime_r(&now, &tmStruct);

    #endif

    return strftime(dateTimeString, size, "%d-%m-%Y %H:%M:%S", &tmStruct);
}
