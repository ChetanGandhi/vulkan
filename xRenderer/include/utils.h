#pragma once

#include "platform.h"

#ifndef NDEBUG

#define CHECK_ERROR(result) xr::checkError(result, __FILE__, __LINE__);

#else

#define CHECK_ERROR(result) ((void)0)

#endif

namespace xr {
    XR_API void checkError(const VkResult result, const char* file, const uint32_t lineNumber);
    XR_API uint32_t findMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties *gpuMemoryProperties, const VkMemoryRequirements *memoryRequirements, const VkMemoryPropertyFlags memoryPropertyFlags);
    XR_API bool readFile(const char* fileName, std::vector<char> *data);
    XR_API size_t currentDateTime(char *dateTimeString, size_t size);
}
