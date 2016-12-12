#pragma once

#include "platform.h"
#include <assert.h>
#include <iostream>

void checkError(VkResult result);

uint32_t findMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties *gpuMemoryProperties, const VkMemoryRequirements *memoryRequirements, const VkMemoryPropertyFlags memoryPropertyFlags);
