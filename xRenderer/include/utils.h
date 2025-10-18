#pragma once

#include "platform.h"

XR_API bool xrReadFile(const char *fileName, char **data, size_t *fileSize);
XR_API size_t xrCurrentDateTime(char *dateTimeString, size_t size);
