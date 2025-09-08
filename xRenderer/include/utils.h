#pragma once

#include "platform.h"

#define XR_IS_ERROR(result) (result != VK_SUCCESS)
#define XR_CHECK_RESULT(result, expected) (result == expected)

XR_API char *xrErrorName(const VkResult result);
XR_API bool xrReadFile(const char *fileName, std::vector<char> *data);
XR_API size_t xrCurrentDateTime(char *dateTimeString, size_t size);
