#pragma once

#include "platform.h"
#include <assert.h>
#include <iostream>
#include <string>

void checkError(VkResult result, std::string file, uint32_t lineNumber);

uint32_t findMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties *gpuMemoryProperties, const VkMemoryRequirements *memoryRequirements, const VkMemoryPropertyFlags memoryPropertyFlags);
