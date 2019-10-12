#pragma once

#include "platform.h"

#ifndef NDEBUG

#define CHECK_ERROR(result) checkError(result, __FILE__, __LINE__);

#else

#define CHECK_ERROR(result) ((void)0)

#endif

void checkError(VkResult result, std::string file, uint32_t lineNumber);

uint32_t findMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties *gpuMemoryProperties, const VkMemoryRequirements *memoryRequirements, const VkMemoryPropertyFlags memoryPropertyFlags);

bool readFile(const std::string &fileName, std::vector<char> *data);

size_t currentDateTime(char *dateTimeString, size_t size);
