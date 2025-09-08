#include "platform.h"
#include "utils.h"

#define XR_ERROR_NAME(error, result) "[" #error "|" #result "]"

XR_API char *xrErrorName(const VkResult result)
{
    if (result < 0)
    {
        switch (result)
        {
            case VK_ERROR_OUT_OF_HOST_MEMORY: // -1
                return XR_ERROR_NAME("VK_ERROR_OUT_OF_HOST_MEMORY", result);

            case VK_ERROR_OUT_OF_DEVICE_MEMORY: // -2
                return XR_ERROR_NAME("VK_ERROR_OUT_OF_DEVICE_MEMORY", result);

            case VK_ERROR_INITIALIZATION_FAILED: // -3
                return XR_ERROR_NAME("VK_ERROR_INITIALIZATION_FAILED", result);

            case VK_ERROR_DEVICE_LOST: // -4
                return XR_ERROR_NAME("VK_ERROR_DEVICE_LOST", result);

            case VK_ERROR_MEMORY_MAP_FAILED: // -5
                return XR_ERROR_NAME("VK_ERROR_MEMORY_MAP_FAILED", result);

            case VK_ERROR_LAYER_NOT_PRESENT: // -6
                return XR_ERROR_NAME("VK_ERROR_LAYER_NOT_PRESENT", result);

            case VK_ERROR_EXTENSION_NOT_PRESENT: // -7
                return XR_ERROR_NAME("VK_ERROR_EXTENSION_NOT_PRESENT", result);

            case VK_ERROR_FEATURE_NOT_PRESENT: // -8
                return XR_ERROR_NAME("VK_ERROR_FEATURE_NOT_PRESENT", result);

            case VK_ERROR_INCOMPATIBLE_DRIVER: // -9
                return XR_ERROR_NAME("VK_ERROR_INCOMPATIBLE_DRIVER", result);

            case VK_ERROR_TOO_MANY_OBJECTS: // -10
                return XR_ERROR_NAME("VK_ERROR_TOO_MANY_OBJECTS", result);

            case VK_ERROR_FORMAT_NOT_SUPPORTED: // -11
                return XR_ERROR_NAME("VK_ERROR_FORMAT_NOT_SUPPORTED", result);

            case VK_ERROR_SURFACE_LOST_KHR: // -1000000000
                return XR_ERROR_NAME("VK_ERROR_SURFACE_LOST_KHR", result);

            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: // -1000000001
                return XR_ERROR_NAME("VK_ERROR_NATIVE_WINDOW_IN_USE_KHR", result);

            case VK_ERROR_OUT_OF_DATE_KHR: // -1000001004
                return XR_ERROR_NAME("VK_ERROR_OUT_OF_DATE_KHR", result);

            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: // -1000003001
                return XR_ERROR_NAME("VK_ERROR_INCOMPATIBLE_DISPLAY_KHR", result);

            case VK_ERROR_VALIDATION_FAILED_EXT: // -1000011001
                return XR_ERROR_NAME("VK_ERROR_VALIDATION_FAILED_EXT", result);

            case VK_ERROR_INVALID_SHADER_NV: // -1000012000
                return XR_ERROR_NAME("VK_ERROR_INVALID_SHADER_NV", result);

            default:
                return XR_ERROR_NAME("Unknown Error", result);
        }
    }

    return "";
}

XR_API bool xrReadFile(const char *fileName, std::vector<char> *data)
{
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        std::cout << "Unable to open file: " << fileName << "\n";
        return false;
    }

    size_t fileSize = (size_t)file.tellg();
    data->resize(fileSize);

    file.seekg(0);
    file.read(data->data(), fileSize);
    file.close();

    return true;
}

XR_API size_t xrCurrentDateTime(char *dateTimeString, size_t size)
{
    time_t now = time(NULL);
    struct tm tmStruct;

#if defined(_WIN32) // check for Windows

    _localtime64_s(&tmStruct, &now);

#elif defined(__linux) // check for Linux

    localtime_r(&now, &tmStruct);

#endif

    return strftime(dateTimeString, size, "%d-%m-%Y %H:%M:%S", &tmStruct);
}
